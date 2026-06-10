#include <stdio.h>
#include <stdlib.h>

#include "profiler.h"
#include "utils.h"

int main() {
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("Hello, World!\n");

    int *ptr = (int*)malloc(sizeof(int));
    int *ptr1 = (int*)malloc(sizeof(int));

    free(ptr);

    printf("Hello, World!\n");

    return 0;
}