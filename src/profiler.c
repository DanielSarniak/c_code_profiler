#include <dlfcn.h>
#include <string.h>
#include <unistd.h>
#include <string.h>

#include "profiler.h"
#include "utils.h"

static void* (*real_malloc)(size_t) = NULL;

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

    LOG("Hook to real_malloc found");
}

static void init_orig_functions() {
    if (real_malloc) return;
    
    hooks_initializing = 1;

     *(void **)(&real_malloc) = dlsym(RTLD_NEXT, "malloc");

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
    // char *txt = "Malloc body\n";
    // write(1, txt, strlen(txt));

    void *ptr = real_malloc(size);

    if(ptr && ptr != (void*)&bootstrap_buffer)
    {
        profiler_add((uintptr_t)ptr, size);
    }
    return ptr;
}