#ifndef PRINT3_SCENE_H_
#define PRINT3_SCENE_H_

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

#include "raylib.h"

#define da_add(da, item)                                                             \
    do {                                                                             \
        if ((da).length >= (da).capacity) {                                          \
            (da).capacity = (da).capacity ? 2 * (da).capacity : 4;                   \
            (da).items = realloc((da).items, (da).capacity * sizeof((da).items[0])); \
            assert((da).items && "Could not resize dynamic array.");                 \
        }                                                                            \
        (da).items[(da).length++] = item;                                            \
    } while (0)

#define da_add3(da, item1, item2, item3) \
    da_add(da, item1);                   \
    da_add(da, item2);                   \
    da_add(da, item3)

typedef struct Vertices {
    Vector3 *items;
    size_t length;
    size_t capacity;
} Vertices;

typedef struct VertexIndices {
    size_t *items;
    size_t length;
    size_t capacity;
} VertexIndices;

typedef struct Object {
    VertexIndices surface;
    Color color;
} Object;

typedef struct Objects {
    Object *items;
    size_t length;
    size_t capacity;
} Objects;

typedef struct Scene {
    Vertices vertices;
    Objects objects;
} Scene;

void scene_free_members(Scene *scene);

void scene_add_demo_object(Scene *scene);

#endif
