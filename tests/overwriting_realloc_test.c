#include <stdlib.h>

int main() {
    int *arr = malloc(10 * sizeof(int));

    arr = realloc(arr, 20 * sizeof(int));

    arr = malloc(30 * sizeof(int));

    free(arr);
}