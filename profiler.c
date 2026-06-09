#include <dlfcn.h>
#include <string.h>
#include <unistd.h>
#include <string.h>

#include "profiler.h"


__attribute__((constructor))
void init_profiler(void) {
    real_malloc = dlsym(RTLD_NEXT, "malloc");

    char *txt = "Hook to real_malloc found\n";
    write(1, txt, strlen(txt));
}

static void init_orig_functions() {
    if (real_malloc) return;
    
    real_malloc  = dlsym(RTLD_NEXT, "malloc");
}

int profiler_add(uintptr_t addr, size_t size)
{
    if (addr == 0)
    {
        char *txt = "[ERROR] Try to alloc to NULL!\n";
        write(1, txt, strlen(txt));
        return 0;
    }

    size_t index = hash_address(addr);

    if( !real_malloc )
    {
        char *txt = "[ERROR] Real malloc never found!\n";
        write(1, txt, strlen(txt));
        return 0;
    }
    AllocationNode* node = (AllocationNode*)real_malloc(sizeof(AllocationNode));
    if (!node)
    {
        char *txt = "[ERROR] Real malloc failed!\n";
        write(1, txt, strlen(txt));
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