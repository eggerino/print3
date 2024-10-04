#ifndef PRINT3_DESERIALIZE_PLY_H_
#define PRINT3_DESERIALIZE_PLY_H_

#include "../scene.h"

void ply_deserialize(void *buffer, size_t size, Color fallback_color, Scene *scene);

#endif
