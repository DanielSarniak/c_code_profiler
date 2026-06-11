#include <stdlib.h>

int main() {
    int *ptr = calloc(10, sizeof(int));

    ptr = calloc(20, sizeof(int));

    free(ptr);

    return 0;
}