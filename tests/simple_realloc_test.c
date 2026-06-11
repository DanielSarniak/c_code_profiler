#include <stdio.h>
#include <stdlib.h>

#include "profiler.h"
#include "utils.h"

int main() {
    int *ptr = (int*)malloc(sizeof(int));

    ptr = realloc(ptr, 7 * sizeof(int));

    return 0;
}