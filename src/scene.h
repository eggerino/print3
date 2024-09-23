#ifndef PRINT3_SCENE_H_
#define PRINT3_SCENE_H_

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

#include "dsa.h"
#include "raylib.h"

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

typedef struct Colors {
    Color *items;
    size_t length;
    size_t capacity;
} Colors;

typedef struct Object {
    VertexIndices surface;
    Colors colors;
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
