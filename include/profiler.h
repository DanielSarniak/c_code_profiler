#ifndef PROFILER_LIB_H
#define PROFILER_LIB_H

#include <stdint.h>
#include <stddef.h>
#include <pthread.h>
#include <stdio.h>

#define HASH_MAP_SIZE   1021
#define MAX_USER_RANGES 16

typedef struct AllocationEntry {
    uintptr_t address;
    size_t size;
    struct AllocationEntry* next;
} AllocationEntry;

typedef struct {
    AllocationEntry* buckets[HASH_MAP_SIZE];
    pthread_mutex_t locks[HASH_MAP_SIZE];
    
    size_t total_allocated;
    size_t peak_allocated;
    pthread_mutex_t stats_lock;
} ProfilerMap;

typedef struct {
    uintptr_t start;
    uintptr_t end;
} UserMemRange;

static UserMemRange user_ranges[MAX_USER_RANGES];
static int user_ranges_count = 0;

static inline size_t hash_address(uintptr_t address) {
    return ((address >> 4) ^ (address >> 12)) % HASH_MAP_SIZE;
}


void* malloc(size_t size);
void free(void* ptr);

void init_profiler(void);
void finalize_profiler(void);
int profiler_add(uintptr_t addr, size_t size);
int profiler_remove(uintptr_t addr);

#endif /* PROFILER_LIB_H */