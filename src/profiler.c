#define _GNU_SOURCE
#include <dlfcn.h>
#include <execinfo.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "profiler.h"
#include "utils.h"

static void* (*real_malloc)(size_t) = NULL;
static void (*real_free)(void*) = NULL;
static void* (*real_calloc)(size_t, size_t) = NULL;
static void* (*real_realloc)(void*, size_t) = NULL;

/* We need to replace real malloc etc. functions with
   our implementations, before  any other lib is loaded,
   but we want to use dlfcn lib here, so we make static
   buffer dedicated to that lib in case it needs to use malloc*/
static int hooks_initializing = 0;
static char bootstrap_buffer[4096];
static size_t bootstrap_used = 0;
static __thread int internal_memory_usage = 0;

static int is_bootstrap_ptr(void* ptr) {
    return (ptr >= (void*)bootstrap_buffer && 
            ptr < (void*)(bootstrap_buffer + sizeof(bootstrap_buffer)));
}

const char* const ignored_symbols[] = {
    "_IO_",
    "printf",
    "vfprintf",
    "puts",
    "_dl_",
    "dlopen",
    "pthread_",
    "setlocale",
    "getpw",
    "getaddrinfo",
    NULL
};


__attribute__((constructor))
void init_profiler(void) {
    *(void **)(&real_malloc) = dlsym(RTLD_NEXT, "malloc");
    *(void **)(&real_free) = dlsym(RTLD_NEXT, "free");
    *(void **)(&real_calloc)  = dlsym(RTLD_NEXT, "calloc");
    *(void **)(&real_realloc)  = dlsym(RTLD_NEXT, "realloc");
}

static void init_orig_functions() {
    if (real_malloc && real_free && real_calloc && real_realloc)
    {
        return;
    }
    
    hooks_initializing = 1;

    *(void **)(&real_malloc) = dlsym(RTLD_NEXT, "malloc");
    *(void **)(&real_free) = dlsym(RTLD_NEXT, "free");
    *(void **)(&real_calloc)  = dlsym(RTLD_NEXT, "calloc");
    *(void **)(&real_realloc)  = dlsym(RTLD_NEXT, "realloc");

    hooks_initializing = 0;
}

static void* bootstrap_malloc(size_t size) {

    size = (size + 7) & ~7; // alignment to 8bits
    if (bootstrap_used + size > sizeof(bootstrap_buffer)) {
        ERROR("Bootstrap buffer overflow!\n");
        return NULL;
    }
    void* ptr = &bootstrap_buffer[bootstrap_used];
    bootstrap_used += size;
    return ptr;
}

static int is_glibc_internal_alloc(void) {
void* caller = __builtin_return_address(1);
    if (!caller) return 0;

    Dl_info info;
    if (dladdr(caller, &info) != 0 && info.dli_sname != NULL) {
        for (int i = 0; ignored_symbols[i] != NULL; i++) {
            if (strstr(info.dli_sname, ignored_symbols[i]) != NULL) {
                return 1;
            }
        }
    }

    return 0;
}

int profiler_add(uintptr_t addr, size_t size)
{
    if (addr == 0)
    {
        ERROR("Try to alloc to NULL!");
        return 0;
    }

    if( !real_malloc )
    {
        ERROR("Real malloc never found!");
        return 0;
    }

    AllocationEntry* node = (AllocationEntry*)real_malloc(sizeof(AllocationEntry));
    if (!node)
    {
        ERROR("Real malloc failed!");
        return 0;
    }

    size_t index = hash_address(addr);

    node->address = addr;
    node->size = size;

    node->next = profiler.buckets[index];
    profiler.buckets[index] = node;

    profiler.total_allocated += size;
    if (profiler.total_allocated > profiler.peak_allocated) {
        profiler.peak_allocated = profiler.total_allocated;
    }
    return 1;
}

int profiler_remove(uintptr_t addr) {
    if (addr == 0)
    {
        ERROR("Try to free NULL address!");
        return 0;
    }

    if( !real_free )
    {
        ERROR("Real free never found!");
        return 0;
    }

    size_t index = hash_address(addr);
    
    AllocationEntry* curr = profiler.buckets[index];
    AllocationEntry* prev = NULL;
    size_t freed_size = 0;
    int isMemAlloc = 0;

    while (curr != NULL) {
        if (curr->address == addr) {
            freed_size = curr->size;
            isMemAlloc = 1;
            
            if (prev == NULL) {
                profiler.buckets[index] = curr->next;
            } else {
                prev->next = curr->next;
            }

            real_free(curr);
            break;
        }
        prev = curr;
        curr = curr->next;
    }

    if (isMemAlloc) {
        profiler.total_allocated -= freed_size;
    } else {
        WARN("Attempted free on unregistered address!\n");
    }
    return 1;
}

__attribute__((destructor))
void finalize_profiler(void) 
{
    PRINT("\n\n========================================\n");
    PRINT("      C_CODE_PROFILER REPORT             \n");
    PRINT("========================================\n");

    size_t leak_count = 0;
    size_t total_leaked_bytes = 0;

    for (int i = 0; i < HASH_MAP_SIZE; i++) {

        AllocationEntry* curr = profiler.buckets[i];
        while (curr != NULL) {
            leak_count++;
            total_leaked_bytes += curr->size;

            PRINT("[LEAK] Address: 0x");
            print_num(curr->address, 16);
            PRINT(" | Size: ");
            print_num(curr->size, 10);
            PRINT(" bytes\n");

            AllocationEntry* next = curr->next;
            real_free(curr); 
            curr = next;
        }
        
    }

    PRINT("----------------------------------------\n");
    PRINT("Total leaks found: ");
    print_num(leak_count, 10);
    PRINT("\n");

    PRINT("Total memory leaked: ");
    print_num(total_leaked_bytes, 10);
    PRINT(" bytes\n");

    PRINT("Peak memory usage: ");
    print_num(profiler.peak_allocated, 10);
    PRINT(" bytes\n");
    PRINT("========================================\n");
}

void* malloc(size_t size) {
    if (internal_memory_usage) {
        return real_malloc ? real_malloc(size) : bootstrap_malloc(size);
    }

    if (!real_malloc) {
        if( hooks_initializing )
        {
            return bootstrap_malloc(size);
        }
        init_orig_functions();
    }

    internal_memory_usage = 1;

    void *ptr = real_malloc(size);

    if(ptr && ptr != (void*)&bootstrap_buffer && !is_glibc_internal_alloc())
    {
        LOG("ADD PROFILER");
        if( !profiler_add((uintptr_t)ptr, size) )
        {
            ERROR("Error occurred during memory allocation");
            exit(1);
        }
    }
    internal_memory_usage = 0;
    return ptr;
}


void free(void* ptr) {
    if( internal_memory_usage )
    {
        if(real_free)
        {
            real_free(ptr);
        }
        return;
    }

    if (!real_free) {
        init_orig_functions();
    }

    if (ptr == NULL)
    {
        return;
    }

    if (is_bootstrap_ptr(ptr))
    {
        return;
    }

    internal_memory_usage = 1;

    if( !profiler_remove((uintptr_t)ptr) )
    {
        ERROR("Error occurred during freeing memory");
        exit(1);
    }

    internal_memory_usage = 0;

    real_free(ptr);
}

void* calloc(size_t nmemb, size_t size) {
    if(internal_memory_usage) {
        if (real_calloc) return real_calloc(nmemb, size);
        size_t total = nmemb * size;
        void* ptr = bootstrap_malloc(total);
        if (ptr) {
            char* p = (char*)ptr;
            for (size_t i = 0; i < total; i++) p[i] = 0;
        }
        return ptr;
    }

    if (!real_calloc) {
        if (hooks_initializing) {
            size_t total = nmemb * size;
            void* ptr = bootstrap_malloc(total);
            if (ptr) {
                char* p = (char*)ptr;
                for (size_t i = 0; i < total; i++) p[i] = 0;
            }
            return ptr;
        }
        init_orig_functions();
    }

    internal_memory_usage = 1;

    void* ptr = real_calloc(nmemb, size);

    if (ptr && !is_glibc_internal_alloc()) {
        if( !profiler_add((uintptr_t)ptr, nmemb * size) )
        {
            ERROR("Error occurred during memory allocation");
            exit(1);
        }
    }

    internal_memory_usage - 0;

    return ptr;
}

void* realloc(void* ptr, size_t size) {
    if (internal_memory_usage) {
        return real_realloc ? real_realloc(ptr, size) : bootstrap_malloc(size);
    }

    if (!real_realloc) {
        if (hooks_initializing) {
            if (ptr == NULL) 
            {
                return bootstrap_malloc(size);
            }
            void* new_ptr = bootstrap_malloc(size);
            if (new_ptr && ptr) {
                memcpy(new_ptr, ptr, size);
            }
            return new_ptr;
        }
        init_orig_functions();
    }

    if (ptr == NULL) {
        return malloc(size);
    }
    if (size == 0) {
        free(ptr);
        return NULL;
    }

    if (is_bootstrap_ptr(ptr)) {
        void* new_ptr = malloc(size);
        if (new_ptr) {
            memcpy(new_ptr, ptr, size);
        }
        return new_ptr;
    }

    internal_memory_usage = 1;

    void* new_ptr = real_realloc(ptr, size);

    if (new_ptr) {
        if( !profiler_remove((uintptr_t)ptr) )
        {
            ERROR("Error occurred during freeing memory");
            exit(1);
        }
        
        if(!is_glibc_internal_alloc())
        {
            if( !profiler_add((uintptr_t)new_ptr, size) )
            {
                ERROR("Error occurred during memory allocation");
                exit(1);
            }
        }
    }

    internal_memory_usage = 0;

    return new_ptr;
}