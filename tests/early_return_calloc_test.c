#include <stdlib.h>

int process(int x) {
    int *arr = calloc(15, sizeof(int));

    if (arr == NULL)
        return -1;

    if (x < 0)
        return 0;

    free(arr);
    return 1;
}

int main() {
    process(-5);
}