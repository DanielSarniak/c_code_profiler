#include <pthread.h>
#include <stdlib.h>

#define THREADS 16
#define OPS 100000

void* worker(void* arg)
{
    for (int i = 0; i < OPS; i++)
    {
        void* p = malloc(rand() % 512 + 1);
        free(p);
    }

    return NULL;
}

int main(void)
{
    pthread_t threads[THREADS];

    for (int i = 0; i < THREADS; i++)
        pthread_create(&threads[i], NULL, worker, NULL);

    for (int i = 0; i < THREADS; i++)
        pthread_join(threads[i], NULL);

    return 0;
}