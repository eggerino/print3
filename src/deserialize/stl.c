#include "stl.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "raymath.h"

static void ascii_stl_deserialize(void *buffer, size_t size, Color fallback_color, Scene *scene);

// binary implementation
static void bin_stl_deserialize(void *buffer, size_t size, Color fallback_color, Scene *scene);
static uint32_t bin_read_u32(uint8_t *ptr);
static float bin_read_f32(uint8_t *ptr);

void stl_deserialize(void *buffer, size_t size, Color fallback_color, Scene *scene) {
    // determine it is binary or ascii format

    // First 80 byte is the header in binary
    // Ascii starts with solid keyword and binary header is permitted to start with same bytes
    if (strncmp(buffer, "solid", 5) == 0) {
        ascii_stl_deserialize(buffer, size, fallback_color, scene);
    } else {
        bin_stl_deserialize(buffer, size, fallback_color, scene);
    }
}

#include <assert.h>

void ascii_stl_deserialize(void *buffer, size_t size, Color fallback_color, Scene *scene) { assert("TODO"); }

// ****************************************************************************
// binary implementation
// ****************************************************************************

void bin_stl_deserialize(void *buffer, size_t size, Color fallback_color, Scene *scene) {
    uint8_t *ptr = buffer;

    // Skip header
    ptr = &ptr[80];

    // Read number of facets
    uint32_t facet_count = bin_read_u32(ptr);
    ptr += 4;

    // Check if file size matches the computed size
    size_t computed_size = 80 + 4 + facet_count * (12 * 4 + 2);
    if (size != computed_size) {
        fprintf(stderr, "[ERR] Invalid format. File has %u bytes but according to the content should have %u bytes.\n", size,
                computed_size);
        exit(1);
    }

    // Create a new object
    Object obj = {0};
    obj.color = fallback_color;  // Use fallback color since stl does not support colors

    // add all facets to the the new object
    float coords[12];
    for (size_t i_facet = 0; i_facet < facet_count; ++i_facet) {
        // Read all coordinates for the facet
        for (size_t i_coord = 0; i_coord < 12; ++i_coord) {
            coords[i_coord] = bin_read_f32(ptr);
            ptr += 4;
        }

        // Skip the attribute
        ptr += 2;

        // Place the coordinates in vector3 structs
        Vector3 normal = {coords[0], coords[1], coords[2]};
        Vector3 v1 = {coords[3], coords[4], coords[5]};
        Vector3 v2 = {coords[6], coords[7], coords[8]};
        Vector3 v3 = {coords[9], coords[10], coords[11]};

        // Check if v1, v2, v3 is counter clockwise
        Vector3 v21 = Vector3Subtract(v2, v1);
        Vector3 v31 = Vector3Subtract(v3, v1);
        Vector3 internal_normal = Vector3CrossProduct(v21, v31);

        if (Vector3DotProduct(normal, internal_normal) < 0) {
            // Swap v2 and v3
            Vector3 temp = v2;
            v2 = v3;
            v3 = temp;
        }

        size_t v1_index = scene->vertices.length;
        da_add3(scene->vertices, v1, v2, v3);
        da_add3(obj.surface, v1_index, v1_index + 1, v1_index + 2);
    }

    // Add the created object to the scene
    da_add(scene->objects, obj);
}

uint32_t bin_read_u32(uint8_t *ptr) {
    // little endian
    return ptr[3] << 24 | ptr[2] << 16 | ptr[1] << 8 | ptr[0];
}

float bin_read_f32(uint8_t *ptr) {
    // This only works if c compiler uses IEEE 754
    uint32_t as_u32 = bin_read_u32(ptr);
    return *((float *)&as_u32);
}
