#include <pthread.h>
#include <stdlib.h>

#define COUNT 100000

void* ptrs[COUNT];

void* producer(void* arg)
{
    for (int i = 0; i < COUNT; i++)
        ptrs[i] = malloc(128);

    return NULL;
}

void* consumer(void* arg)
{
    pthread_join(*(pthread_t*)arg, NULL);

    for (int i = 0; i < COUNT; i++)
        free(ptrs[i]);

    return NULL;
}

int main(void)
{
    pthread_t prod;
    pthread_t cons;

    pthread_create(&prod, NULL, producer, NULL);
    pthread_create(&cons, NULL, consumer, &prod);

    pthread_join(cons, NULL);

    return 0;
}