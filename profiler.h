#ifndef PROFILER_LIB_H
#define PROFILER_LIB_H

#include <stdint.h>
#include <stddef.h>
#include <pthread.h>
#include <stdio.h>

#define HASH_MAP_SIZE   1021

static void* (*real_malloc)(size_t) = NULL;

/* We need to replace real malloc etc. functions with
   our implementations, before  any other lib is loaded,
   but we want to use dlfcn lib here, so we make static
   buffer dedicated to that lib in case it need to use malloc*/
static int hooks_initializing = 0;
static char bootstrap_buffer[4096];
static size_t bootstrap_used = 0;

typedef struct AllocationEntry {
    uintptr_t address;
    size_t size;
    struct AllocationEntry* next;
} AllocationNode;

typedef struct {
    AllocationNode* buckets[HASH_MAP_SIZE];
    
    size_t total_allocated;
    size_t peak_allocated;
} ProfilerMap;

static ProfilerMap profiler = {0};

static inline size_t hash_address(uintptr_t address) {
    return ((address >> 4) ^ (address >> 12)) % HASH_MAP_SIZE;
}


void* malloc(size_t size);


static void init_orig_functions();

void init_profiler(void);
int profiler_add(uintptr_t addr, size_t size);

#endif /* PROFILER_LIB_H */