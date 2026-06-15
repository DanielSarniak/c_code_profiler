#include <pthread.h>
#include <stdlib.h>

#define THREADS 10
#define OPS 10

void* worker(void* arg)
{
    long id = (long)arg;

    for (int i = 0; i < OPS; i++)
    {
        void* p = malloc(1024);

        if (id % 2 == 0)
            free(p);
    }

    return NULL;
}

int main(void)
{
    pthread_t threads[THREADS];

    for (long i = 0; i < THREADS; i++)
        pthread_create(&threads[i], NULL, worker, (void*)i);

    for (int i = 0; i < THREADS; i++)
        pthread_join(threads[i], NULL);

    return 0;
}