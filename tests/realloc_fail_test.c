#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int main() {
    size_t huge = 9223372036854775807;
    char *p = malloc(100);

    p = realloc(p, huge);

}