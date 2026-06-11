#include <stdlib.h>

char *buffer;

int main() {
    buffer = calloc(1000, sizeof(int));

    return 0;
}