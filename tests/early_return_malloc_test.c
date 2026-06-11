#include <stdlib.h>

int foo(int error) {
    char *buf = malloc(1024);

    if (error)
        return -1;

    free(buf);
    return 0;
}

int main()
{
    foo(1);
}