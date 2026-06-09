#include <stdio.h>
#include <stdlib.h>

#include "profiler.h"
#include "utils.h"

int main() {
  
    printf("Hello, World!\n");

    int *ptr = (int*)malloc(sizeof(int));

    free(ptr);

    return 0;
}