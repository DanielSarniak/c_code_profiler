#include <stdlib.h>

void foo() {
    char *buffer = calloc(3, sizeof(char));
    if (buffer == NULL)
        return;
    return;
}

int main() {
    foo();
    return 0;
}