#ifndef PRINT3_DESERIALIZE_UTIL_H_
#define PRINT3_DESERIALIZE_UTIL_H_

#include <stdint.h>

#include "raylib.h"

typedef enum ByteOrdering {
    ORDERING_LITTLE_ENDIAN = 0,
    ORDERING_BIG_ENDIAN,
    ByteOrdering_COUNT,
} ByteOrdering;

#define advance_or_err(ptr, peak, ...)    \
    do {                                  \
        if ((peak) == (ptr)) {            \
            fprintf(stderr, __VA_ARGS__); \
            exit(1);                      \
        }                                 \
        (ptr) = (peak);                   \
    } while (0)

#define vector3_at(items, index) \
    (Vector3) { (items)[3 * (index)], (items)[3 * (index) + 1], (items)[3 * (index) + 2] }

#define color_at(items, index) \
    (Color) { (items)[4 * (index)], (items)[4 * (index) + 1], (items)[4 * (index) + 2], (items)[4 * (index) + 3] }

uint8_t binary_buffer_to_u8(const uint8_t *buffer);
uint16_t binary_buffer_to_u16(const uint8_t *buffer, ByteOrdering ordering);
uint32_t binary_buffer_to_u32(const uint8_t *buffer, ByteOrdering ordering);
uint64_t binary_buffer_to_u64(const uint8_t *buffer, ByteOrdering ordering);
float binary_buffer_to_f32_IEEE754(const uint8_t *buffer, ByteOrdering ordering);
double binary_buffer_to_f64_IEEE754(const uint8_t *buffer, ByteOrdering ordering);

char *str_skip(char *str, const char *skip);
char *str_skip_whitespace(char *str);
bool order_vertices(const Vector3 *normal, Vector3 *v1, Vector3 *v2, Vector3 *v3);

#endif
