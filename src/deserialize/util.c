#include "util.h"

#include <assert.h>
#include <string.h>

uint32_t binary_buffer_to_u32(const uint8_t *buffer, ByteOrdering ordering) {
    static_assert(ByteOrdering_COUNT == 2);
    if (ordering == ORDERING_LITTLE_ENDIAN) {
        return buffer[3] << 24 | buffer[2] << 16 | buffer[1] << 8 | buffer[0];
    } else {
        return buffer[0] << 24 | buffer[1] << 16 | buffer[2] << 8 | buffer[3];
    }
}

float binary_buffer_to_f32_IEEE754(const uint8_t *buffer, ByteOrdering ordering) {
    uint32_t as_u32 = binary_buffer_to_u32(buffer, ordering);

#if defined(__STDC_IEC_559__)
    return *((float *)&as_u32);
#endif
}

char *str_skip(char *str, const char *skip) {
    size_t n = strlen(skip);
    char *at_skip = strstr(str, skip);

    // Only add n (after skip string) if it was found
    return at_skip ? at_skip + n : NULL;
}
