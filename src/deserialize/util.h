#ifndef PRINT3_DESERIALIZE_UTIL_H_
#define PRINT3_DESERIALIZE_UTIL_H_

#include <stdint.h>

typedef enum ByteOrdering {
    ORDERING_LITTLE_ENDIAN = 0,
    ORDERING_BIG_ENDIAN,
    ByteOrdering_COUNT,
} ByteOrdering;

uint32_t binary_buffer_to_u32(const uint8_t *buffer, ByteOrdering ordering);
float binary_buffer_to_f32_IEEE754(const uint8_t *buffer, ByteOrdering ordering);
char *str_skip(char *str, const char *skip);

#endif
