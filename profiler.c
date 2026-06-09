#include <dlfcn.h>
#include <string.h>
#include <unistd.h>
#include <string.h>

#include "profiler.h"


__attribute__((constructor))
void init_profiler(void) {
    real_malloc = dlsym(RTLD_NEXT, "malloc");

    const char *err = "Hook to real_malloc found\n";
    write(1, err, strlen(err));
}

static void init_orig_functions() {
    if (real_malloc) return;
    
    hooks_initializing = 1;

    real_malloc  = dlsym(RTLD_NEXT, "malloc");

    hooks_initializing = 0;
}

static void* bootstrap_malloc(size_t size) {

    size = (size + 7) & ~7; // alignment to 8bits
    if (bootstrap_used + size > sizeof(bootstrap_buffer)) {
        const char *err = "[ERROR] Bootstrap buffer overflow!\n";
        write(2, err, sizeof(err) - 1);
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
        const char *err = "[ERROR] Try to alloc to NULL!\n";
        write(2, err, strlen(err));
        return 0;
    }

    size_t index = hash_address(addr);

    if( !real_malloc )
    {
        const char *err = "[ERROR] Real malloc never found!\n";
        write(2, err, strlen(err));
        return 0;
    }
    AllocationNode* node = (AllocationNode*)real_malloc(sizeof(AllocationNode));
    if (!node)
    {
        const char *err = "[ERROR] Real malloc failed!\n";
        write(2, err, strlen(err));
        return 0;
    }

    node->address = addr;
    node->size = size;

    node->next = profiler.buckets[index];
    profiler.buckets[index] = node;

    profiler.total_allocated += size;
    if (profiler.total_allocated > profiler.peak_allocated) {
        profiler.peak_allocated = profiler.total_allocated;
    }
}

void* malloc(size_t size) {
    if (!real_malloc) {
        if( hooks_initializing )
        {
            return bootstrap_malloc(size);
        }
        init_orig_functions();
    }
    char *txt = "Malloc body\n";
    write(1, txt, strlen(txt));

    void *ptr = real_malloc(size);

    if(ptr)
    {
        profiler_add((uintptr_t)ptr, size);
    }
    return ptr;
}