#ifndef PRINT3_DESERIALZE_MEMORY_H_
#define PRINT3_DESERIALZE_MEMORY_H_

#include <stddef.h>

#include "../scene.h"

typedef void (*MemoryDeserializer)(void *buffer, size_t size, Scene *scene);

#endif
