#include <stdio.h>
#include <stdlib.h>

#include "hooks.h"

int main() {
  
    printf("Hello, World!\n");

    int *ptr =  (int*)malloc(sizeof(int));

    free(ptr);

    return 0;
}