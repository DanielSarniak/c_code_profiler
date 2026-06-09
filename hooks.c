#include <dlfcn.h>
#include <unistd.h>
#include <string.h>

#include "hooks.h"

void* malloc(size_t size) {
    if (!real_malloc) {
        real_malloc = dlsym(RTLD_NEXT, "malloc");
        char *txt = "Hook to real_malloc found\n";
        write(1, txt, strlen(txt));
    }
    char *txt = "Malloc body\n";
    write(1, txt, strlen(txt));

    return real_malloc(size);
}