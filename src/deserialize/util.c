#include "util.h"

#include <assert.h>
#include <ctype.h>
#include <string.h>

#include "raymath.h"

uint8_t binary_buffer_to_u8(const uint8_t *buffer) { return *buffer; }

uint16_t binary_buffer_to_u16(const uint8_t *buffer, ByteOrdering ordering) {
    static_assert(ByteOrdering_COUNT == 2, "Exhaustiveness");
    if (ordering == ORDERING_LITTLE_ENDIAN) {
        return buffer[1] << 8 | buffer[0];
    } else {
        return buffer[0] << 8 | buffer[1];
    }
}

uint32_t binary_buffer_to_u32(const uint8_t *buffer, ByteOrdering ordering) {
    static_assert(ByteOrdering_COUNT == 2, "Exhaustiveness");
    if (ordering == ORDERING_LITTLE_ENDIAN) {
        return buffer[3] << 24 | buffer[2] << 16 | buffer[1] << 8 | buffer[0];
    } else {
        return buffer[0] << 24 | buffer[1] << 16 | buffer[2] << 8 | buffer[3];
    }
}

uint64_t binary_buffer_to_u64(const uint8_t *buffer, ByteOrdering ordering) {
    static_assert(ByteOrdering_COUNT == 2, "Exhaustiveness");
    uint64_t result = 0;

    for (int i = 0; i < 8; ++i) {
        int shift = ordering == ORDERING_LITTLE_ENDIAN ? 8 * i : 8 * (7 - i);

        result = result | buffer[i] << shift;
    }

    return result;
}

float binary_buffer_to_f32_IEEE754(const uint8_t *buffer, ByteOrdering ordering) {
    uint32_t as_u32 = binary_buffer_to_u32(buffer, ordering);

#if defined(__STDC_IEC_559__)
    return *((float *)&as_u32);
#endif
}

double binary_buffer_to_f64_IEEE754(const uint8_t *buffer, ByteOrdering ordering) {
    uint64_t as_u64 = binary_buffer_to_u64(buffer, ordering);

#if defined(__STDC_IEC_559__)
    return *((double *)&as_u64);
#endif
}

char *str_skip(char *str, const char *skip) {
    size_t n = strlen(skip);
    char *at_skip = strstr(str, skip);

    // Only add n (after skip string) if it was found
    return at_skip ? at_skip + n : NULL;
}

char *str_skip_whitespace(char *str) {
    // proceed in the string until it is at the end or the first non whitespace is found
    while (*str != '\0' && isspace(*str)) ++str;

    return str;
}

bool order_vertices(const Vector3 *normal, Vector3 *v1, Vector3 *v2, Vector3 *v3) {
    // Check if v1, v2, v3 is counter clockwise when looking from the normal direction
    // (view isalong the negative normal vector)
    Vector3 v21 = Vector3Subtract(*v2, *v1);
    Vector3 v31 = Vector3Subtract(*v3, *v1);
    Vector3 internal_normal = Vector3CrossProduct(v21, v31);

    // dot product is positive if both normal vectors point to same direction
    if (Vector3DotProduct(*normal, internal_normal) < 0) {
        // Swap v2 and v3
        Vector3 temp = *v2;
        *v2 = *v3;
        *v3 = temp;
        return true;
    }
    return false;
}
