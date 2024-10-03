#ifndef PRINT3_DESERIALIZE_OBJ_H_
#define PRINT3_DESERIALIZE_OBJ_H_

#include "../scene.h"

void obj_deserialize(void *buffer, size_t size, Color fallback_color, Scene *scene);

#endif
