#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

#define THREADS 16
#define ITEMS 50000

void* ptrs[ITEMS];

pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
int index_alloc = 0;

void* allocator(void* arg)
{
    while (1)
    {
        pthread_mutex_lock(&mtx);

        if (index_alloc >= ITEMS)
        {
            pthread_mutex_unlock(&mtx);
            break;
        }

        int idx = index_alloc++;

        pthread_mutex_unlock(&mtx);

        ptrs[idx] = malloc(128);
    }

    return NULL;
}

int main(void)
{
    pthread_t threads[THREADS];

    for (int i = 0; i < THREADS; i++)
        pthread_create(&threads[i], NULL, allocator, NULL);

    for (int i = 0; i < THREADS; i++)
        pthread_join(threads[i], NULL);

    for (int i = 0; i < ITEMS; i++){
        free(ptrs[i]);
    }

    return 0;
}