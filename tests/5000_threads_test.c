#include <pthread.h>
#include <stdlib.h>

#define THREADS 5000

void* worker(void* arg)
{
    void* p = malloc(256);
    free(p);

    return NULL;
}

int main(void)
{
    pthread_t t;

    for (int i = 0; i < THREADS; i++)
    {
        pthread_create(&t, NULL, worker, NULL);
        pthread_join(t, NULL);
    }

    return 0;
}