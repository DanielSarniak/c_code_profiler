#ifndef HOOKS_LIB_H
#define HOOKS_LIB_H

#include <stdio.h>

static void* (*real_malloc)(size_t) = NULL;


void* malloc(size_t size);

#endif /* HOOKS_LIB_H */