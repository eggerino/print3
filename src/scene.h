#ifndef PRINT3_SCENE_H_
#define PRINT3_SCENE_H_

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

#include "dsa.h"
#include "raylib.h"

#define da_add_vector3(da, vec) da_add3(da, (vec).x, (vec).y, (vec).z)
#define da_add_color(da, color) da_add4(da, (color).r, (color).g, (color).b, (color).a)

typedef struct Vertices {
    float *items;  // 3 per vertex X Y Z
    size_t length;
    size_t capacity;
} Vertices;

typedef struct Colors {
    unsigned char *items;  // 4 per color R G B A
    size_t length;
    size_t capacity;
} Colors;

typedef struct Object {
    Vertices vertices;
    Colors colors;
} Object;

typedef struct Objects {
    Object *items;
    size_t length;
    size_t capacity;
} Objects;

typedef struct Scene {
    Objects objects;
} Scene;

void scene_free_members(Scene *scene);

void scene_add_demo_object(Scene *scene);

#endif
