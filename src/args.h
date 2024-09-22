#ifndef PRINT3_ARGS_H_
#define PRINT3_ARGS_H_

#include <stdbool.h>

#include "viewer.h"

typedef struct Files {
    const char **items;
    size_t length;
    size_t capacity;
} Files;

typedef struct Args {
    int stdin_object_count;
    Files files;
    Color fallback_color;
    ViewerOptions viewer;
} Args;

void args_parse(int argc, const char **argv, Args *args);
void args_free_member(Args *args);
void args_print(const Args *args);

#endif
