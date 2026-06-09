#ifndef PROFILER_LIB_H
#define PROFILER_LIB_H

#include <stdint.h>
#include <stddef.h>
#include <pthread.h>
#include <stdio.h>

#define HASH_MAP_SIZE   1021

static void* (*real_malloc)(size_t) = NULL;

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



int profiler_add(uintptr_t addr, size_t size);

#endif /* PROFILER_LIB_H */