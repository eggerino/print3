#include "stl.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "raymath.h"
#include "util.h"

// scii implementation
static void ascii_stl_deserialize(void *buffer, size_t size, Color fallback_color, Scene *scene);
static void ascii_str_to_vector3(char *ptr, char **end, Vector3 *vector);

// binary implementation
static void bin_stl_deserialize(void *buffer, size_t size, Color fallback_color, Scene *scene);

// common
static void order_vertices(const Vector3 *normal, Vector3 *v1, Vector3 *v2, Vector3 *v3);

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

#define skip_or_err(str, msg)      \
    do {                           \
        peak = str_skip(ptr, str); \
        if (!peak) {               \
            fprintf(stderr, msg);  \
            exit(1);               \
        }                          \
        ptr = peak;                \
    } while (0)

#define vec_or_err(vec_ptr, msg)                   \
    do {                                           \
        ascii_str_to_vector3(ptr, &peak, vec_ptr); \
        if (peak == ptr) {                         \
            fprintf(stderr, msg);                  \
            exit(1);                               \
        }                                          \
        ptr = peak;                                \
    } while (0)

void ascii_stl_deserialize(void *buffer, size_t size, Color fallback_color, Scene *scene) {
    char *ptr = buffer;
    char *peak;

    // Create a new object
    Object obj = {0};

    // Skip the beginning of the solid block
    skip_or_err("solid", "[ERR] Invalid format. No start of a solid block found.\n");
    skip_or_err("\n", "[ERR] Invalid format. No body of a solid block found.\n");

    while (1) {
        // Stop when no mor facet is defined
        peak = str_skip(ptr, "facet normal ");
        if (!peak) break;
        ptr = peak;

        // Parse out the normal vector
        Vector3 normal;
        vec_or_err(&normal, "[ERR] Invalid format. Normal vector is expected but not found.\n");

        // and the vertices
        Vector3 v[3];
        for (int i = 0; i < 3; ++i) {
            skip_or_err("vertex ", "[ERR] Invalid format. Vertex declaration is missing.\n");
            vec_or_err(&v[i], "[ERR] Invalid format. Vertex is missing.\n");
        }

        order_vertices(&normal, &v[0], &v[1], &v[2]);

        // Add the vertices to the scene and the facet to the object
        for (int i = 0; i < 3; ++i) {
            da_add_vector3(obj.vertices, v[i]);
            da_add_color(obj.colors, fallback_color);
        }
    }

    // Add the created object to the scene
    da_add(scene->objects, obj);
}

void ascii_str_to_vector3(char *ptr, char **end, Vector3 *vector) {
    *end = ptr;

    char *peak;
    float comp[3];
    for (size_t i = 0; i < 3; ++i) {
        comp[i] = strtof(ptr, &peak);
        if (peak == ptr) return;
        ptr = peak;
    }

    *end = ptr;
    vector->x = comp[0];
    vector->y = comp[1];
    vector->z = comp[2];
}

// ****************************************************************************
// binary implementation
// ****************************************************************************

void bin_stl_deserialize(void *buffer, size_t size, Color fallback_color, Scene *scene) {
    uint8_t *ptr = buffer;

    // Ensure facet count can be read
    if (size < 84) {
        fprintf(stderr,
                "[ERR] Invalid format. File is too small to be a binary stl file. At least 84 bytes are required but file "
                "has %u bytes.\n",
                size);
        exit(1);
    }

    // Skip header
    ptr = &ptr[80];

    // Read number of facets
    uint32_t facet_count = binary_buffer_to_u32(ptr, ORDERING_LITTLE_ENDIAN);
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

    // add all facets to the the new object
    float coords[12];
    for (size_t i_facet = 0; i_facet < facet_count; ++i_facet) {
        // Read all coordinates for the facet
        for (size_t i_coord = 0; i_coord < 12; ++i_coord) {
            coords[i_coord] = binary_buffer_to_f32_IEEE754(ptr, ORDERING_LITTLE_ENDIAN);
            ptr += 4;
        }

        // Skip the attribute
        ptr += 2;

        // Place the coordinates in vector3 structs
        Vector3 normal = {coords[0], coords[1], coords[2]};
        Vector3 v1 = {coords[3], coords[4], coords[5]};
        Vector3 v2 = {coords[6], coords[7], coords[8]};
        Vector3 v3 = {coords[9], coords[10], coords[11]};

        order_vertices(&normal, &v1, &v2, &v3);

        da_add_vector3(obj.vertices, v1);
        da_add_vector3(obj.vertices, v2);
        da_add_vector3(obj.vertices, v3);

        da_add_color(obj.colors, fallback_color);
        da_add_color(obj.colors, fallback_color);
        da_add_color(obj.colors, fallback_color);
    }

    // Add the created object to the scene
    da_add(scene->objects, obj);
}

// ****************************************************************************
// common
// ****************************************************************************

void order_vertices(const Vector3 *normal, Vector3 *v1, Vector3 *v2, Vector3 *v3) {
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
    }
}
