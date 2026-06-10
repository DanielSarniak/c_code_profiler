#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string.h>

#include "profiler.h"
#include "utils.h"

static void* (*real_malloc)(size_t) = NULL;
static void* (*real_free)(size_t) = NULL;

/* We need to replace real malloc etc. functions with
   our implementations, before  any other lib is loaded,
   but we want to use dlfcn lib here, so we make static
   buffer dedicated to that lib in case it needs to use malloc*/
static int hooks_initializing = 0;
static char bootstrap_buffer[4096];
static size_t bootstrap_used = 0;

__attribute__((constructor))
void init_profiler(void) {
    *(void **)(&real_malloc) = dlsym(RTLD_NEXT, "malloc");
    *(void **)(&real_free) = dlsym(RTLD_NEXT, "free");
}

static void init_orig_functions() {
    if (real_malloc && real_free)
    {
        return;
    }
    
    hooks_initializing = 1;

    *(void **)(&real_malloc) = dlsym(RTLD_NEXT, "malloc");
    *(void **)(&real_free) = dlsym(RTLD_NEXT, "free");

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

            real_free((uintptr_t)curr);
            break;
        }
        prev = curr;
        curr = curr->next;
    }

    if (isMemAlloc) {
        profiler.total_allocated -= freed_size;
    } else {
        WARN("[PROFILER ALERT] Attempted free on unregistered address!\n");
    }
    return 1;
}

void* malloc(size_t size) {
    if (!real_malloc) {
        if( hooks_initializing )
        {
            return bootstrap_malloc(size);
        }
        init_orig_functions();
    }

    LOG("MALLOC USED");
    void *ptr = real_malloc(size);

    if(ptr && ptr != (void*)&bootstrap_buffer)
    {
        if( !profiler_add((uintptr_t)ptr, size) )
        {
            ERROR("Error occurred during memory allocation");
            exit(1);
        }
    }
    return ptr;
}


void free(void* ptr) {
    if (!real_free) {
        init_orig_functions();
    }

    if (ptr == NULL)
    {
        return;
    }

    if (ptr >= (void*)bootstrap_buffer && ptr < (void*)(bootstrap_buffer + sizeof(bootstrap_buffer))) {
        return;
    }

    LOG("FREE USED");
    if( !profiler_remove((uintptr_t)ptr) )
    {
        ERROR("Error occurred during freeing memory");
        exit(1);
    }

    real_free((uintptr_t)ptr);
}