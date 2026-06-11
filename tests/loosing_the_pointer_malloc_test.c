#include <stdlib.h>

int main() {
    int *p = malloc(1000);

    p = malloc(2000);

    free(p);

    return 0;
}