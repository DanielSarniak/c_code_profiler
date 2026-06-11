#include <stdio.h>
#include <stdlib.h>

int main() {
    int *tab = calloc(100, sizeof(int));

    if (tab == NULL) {
        return 1;
    }

    tab[0] = 42;

    // free(tab);

    return 0;
}