#define _GNU_SOURCE
#include <dlfcn.h>
#include <execinfo.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>

#include "profiler.h"
#include "utils.h"

static void* (*real_malloc)(size_t) = NULL;
static void (*real_free)(void*) = NULL;
static void* (*real_calloc)(size_t, size_t) = NULL;
static void* (*real_realloc)(void*, size_t) = NULL;

static ProfilerMap profiler = {0};

/* We need to replace real malloc etc. functions with
   our implementations, before  any other lib is loaded,
   but we want to use dlfcn lib here, so we make static
   buffer dedicated to that lib in case it needs to use malloc*/
static char bootstrap_buffer[4096];
static size_t bootstrap_used = 0;
static _Thread_local int hooks_initializing = 0;
static _Thread_local int internal_memory_usage = 0;
static pthread_once_t init_done = PTHREAD_ONCE_INIT;

static char* app_name = NULL;
static char exe_path[256] = {0};
static uintptr_t libc_start_addr = 0;
static uintptr_t libc_end_addr = 0;

static int is_bootstrap_ptr(void* ptr) {
    return (ptr >= (void*)bootstrap_buffer && 
            ptr < (void*)(bootstrap_buffer + sizeof(bootstrap_buffer)));
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
__attribute__((constructor))
void init_profiler(void) {
    for (int i = 0; i < HASH_MAP_SIZE; i++) {
        pthread_mutex_init(&profiler.locks[i], NULL);
    }
    pthread_mutex_init(&profiler.stats_lock, NULL);

    init_orig_functions();

    internal_memory_usage = 1;
    if (readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1) <= 0) {
        WARN("CAN'T GET USER PROGRAM PATH");
        return;
    }

    app_name  = strrchr(exe_path, '/');
    if (app_name) {
        app_name++;
    } else {
        app_name = exe_path;
    }

    LOG("APP NAME FOUND: ");
    LOG(app_name);

    FILE* f = fopen("/proc/self/maps", "r");
    if (!f)
    {
        WARN("CAN'T OPEN /proc/self/maps");
        return;
    }

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, exe_path)) {
            uintptr_t start, end;
            if (sscanf(line, "%" SCNxPTR "-%" SCNxPTR, &start, &end) == 2) {
                if (user_ranges_count < MAX_USER_RANGES) {
                    user_ranges[user_ranges_count].start = start;
                    user_ranges[user_ranges_count].end = end;
                    user_ranges_count++;
                }
            }
        }
    }
    fclose(f);
    internal_memory_usage = 0;
}

static void* bootstrap_malloc(size_t size) {

    size = (size + 7) & ~7; // alignment to 8 bytes
    if (bootstrap_used + size > sizeof(bootstrap_buffer)) {
        ERROR("Bootstrap buffer overflow!\n");
        return NULL;
    }
    void* ptr = &bootstrap_buffer[bootstrap_used];
    bootstrap_used += size;
    return ptr;
}

static int is_user_code(void) {
    if (user_ranges_count == 0) return 0;

    void* caller = __builtin_return_address(1);
    uintptr_t addr = (uintptr_t)caller;

    for (int i = 0; i < user_ranges_count; i++) {
        if (addr >= user_ranges[i].start && addr < user_ranges[i].end) {
            return 1;
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
    pthread_mutex_lock(&profiler.locks[index]);
    node->address = addr;
    node->size = size;

    node->next = profiler.buckets[index];
    profiler.buckets[index] = node;
    pthread_mutex_unlock(&profiler.locks[index]);

    pthread_mutex_lock(&profiler.stats_lock);
    profiler.total_allocated += size;
    if (profiler.total_allocated > profiler.peak_allocated) {
        profiler.peak_allocated = profiler.total_allocated;
    }
    pthread_mutex_unlock(&profiler.stats_lock);
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
    pthread_mutex_lock(&profiler.locks[index]);
    
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
    pthread_mutex_unlock(&profiler.locks[index]);

    if (isMemAlloc) {
        pthread_mutex_lock(&profiler.stats_lock);
        profiler.total_allocated -= freed_size;
        pthread_mutex_unlock(&profiler.stats_lock);
    } else {
        WARN("Attempted free on unregistered address!\n");
    }
    return 1;
}

__attribute__((destructor))
void finalize_profiler(void) 
{
    PRINT("\n\n ========================================\n");
    PRINT("      C_CODE_PROFILER REPORT             \n");
    PRINT("========================================\n");

    PRINT("Application name: ");
    PRINT(app_name ? app_name : "Unknown");
    PRINT("\n");
    PRINT("Application path: ");
    PRINT(exe_path ? exe_path : "Unknown");
    PRINT("\n\n Application code ranges in memory (function addresses):\n");

    for(int i = 0; i < user_ranges_count; i++)
    {
        PRINT("0x");
        print_num(user_ranges[i].start, 16);
        PRINT(" - 0x");
        print_num(user_ranges[i].end, 16);
        PRINT("\n");
    }

    PRINT("----------------------------------------\n");

    size_t leak_count = 0;
    size_t total_leaked_bytes = 0;

    for (int i = 0; i < HASH_MAP_SIZE; i++) {
        pthread_mutex_lock(&profiler.locks[i]);

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

        pthread_mutex_unlock(&profiler.locks[i]);
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

    for (int i = 0; i < HASH_MAP_SIZE; i++) {
        pthread_mutex_destroy(&profiler.locks[i]);
    }
    pthread_mutex_destroy(&profiler.stats_lock);
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
        pthread_once(&init_done, init_orig_functions);
    }

    internal_memory_usage = 1;

    void *ptr = real_malloc(size);

    if(ptr && ptr != (void*)&bootstrap_buffer && is_user_code())
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
        pthread_once(&init_done, init_orig_functions);
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

    if (is_user_code())
    {
        if( !profiler_remove((uintptr_t)ptr) )
        {
            ERROR("Error occurred during freeing memory");
            exit(1);
        }
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
        pthread_once(&init_done, init_orig_functions);
    }

    internal_memory_usage = 1;

    void* ptr = real_calloc(nmemb, size);

    if (ptr && is_user_code()) {
        if( !profiler_add((uintptr_t)ptr, nmemb * size) )
        {
            ERROR("Error occurred during memory allocation");
            exit(1);
        }
    }

    internal_memory_usage = 0;

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
        pthread_once(&init_done, init_orig_functions);
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
        
        if(is_user_code())
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