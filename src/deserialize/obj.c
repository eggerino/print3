#include "obj.h"

#include <stdio.h>
#include <string.h>

#include "parsing.h"
#include "raymath.h"

typedef struct Floats {
    float *items;
    size_t length;
    size_t capacity;
} Floats;

static char *try_parse_vector(char *ptr, const char *identifier, Floats *vertices);
static char *try_parse_face(char *ptr, const Floats *vertices, const Floats *normals, Color fallback_color, Object *object);
static char *skip_remaining_line(char *ptr);

void obj_deserialize(void *buffer, size_t size, Color fallback_color, Scene *scene) {
    char *ptr = buffer;
    char *peak;

    Floats vertices = {0};
    Floats normals = {0};

    Object object = {0};

    while (*ptr) {
        peak = try_parse_vector(ptr, "v ", &vertices);

        if (peak == ptr) {
            peak = try_parse_vector(ptr, "vn ", &normals);
        }

        if (peak == ptr) {
            peak = try_parse_face(ptr, &vertices, &normals, fallback_color, &object);
        }

        ptr = skip_remaining_line(peak);
    }

    da_add(scene->objects, object);
}

char *try_parse_vector(char *ptr, const char *identifier, Floats *vertices) {
    size_t identifier_length = strlen(identifier);
    if (strncmp(ptr, identifier, identifier_length)) {
        return ptr;
    }

    // Skip the prefix
    ptr += identifier_length;

    // Parse the 3 components
    char *peak;
    float components[3];
    for (int i = 0; i < 3; ++i) {
        components[i] = strtof(ptr, &peak);
        advance_or_err(ptr, peak, "[ERR] Invalid format. %d-th component of the %d-th vertex must be a float.\n", i + 1,
                       vertices->length + i + 1);
    }

    // Parse the optional homongenous component
    float homogenous = strtof(ptr, &peak);
    if (ptr == peak) {
        homogenous = 1.0f;
    } else {
        ptr = peak;
    }

    // Transform into carthesian coordinates and add the vertices
    for (int i = 0; i < 3; ++i) {
        components[i] /= homogenous;
        da_add(*vertices, components[i]);
    }

    return ptr;
}

char *try_parse_face(char *ptr, const Floats *vertices, const Floats *normals, Color fallback_color, Object *object) {
    if (strncmp(ptr, "f ", 2)) {
        return ptr;
    }

    // Skip the prefix
    ptr += 2;

    // Parse the indices of the face
    char *peak;
    bool has_normal = false;
    size_t vertex_indices[3];
    size_t normal_indices[3];
    for (int i = 0; i < 3; ++i) {
        vertex_indices[i] = strtoll(ptr, &peak, 10) - 1;  // One based indexing
        advance_or_err(ptr, peak, "[ERR] Invalid format. %d-th vertex index of the %d-th face must be number.\n", i + 1,
                       object->vertices.length / 9 + 1);

        // Get texture component
        if (*ptr == '/' && ptr[1] != '/') {
            ++ptr;
            strtoll(ptr, &peak, 10);
            advance_or_err(ptr, peak, "[ERR] Invalid format. %d-th texture index of the %d-th face must be number.\n", i + 1,
                           object->vertices.length / 9 + 1);
        }

        // Get normal component
        if (*ptr == '/') {
            ++ptr;
            has_normal = true;
            normal_indices[i] = strtoll(ptr, &peak, 10) - 1;  // One based indexing
            advance_or_err(ptr, peak, "[ERR] Invalid format. %d-th normal index of the %d-th face must be number.\n", i + 1,
                           object->vertices.length / 9 + 1);
        }
    }

    // Extract the vertex vectors from the buffer
    Vector3 v1 = vector3_at(vertices->items, vertex_indices[0]);
    Vector3 v2 = vector3_at(vertices->items, vertex_indices[1]);
    Vector3 v3 = vector3_at(vertices->items, vertex_indices[2]);

    // Consider the normals for vertex ordering
    if (has_normal) {
        Vector3 n1 = vector3_at(normals->items, normal_indices[0]);
        Vector3 n2 = vector3_at(normals->items, normal_indices[1]);
        Vector3 n3 = vector3_at(normals->items, normal_indices[2]);

        Vector3 normal = Vector3Scale(Vector3Add(n1, Vector3Add(n2, n3)), 1.0f / 3.0f);

        order_vertices(&normal, &v1, &v2, &v3);
    }

    // Add the vertices of the face
    da_add_vector3(object->vertices, v1);
    da_add_vector3(object->vertices, v2);
    da_add_vector3(object->vertices, v3);

    // Use the fallback color since .obj does not hold color information
    da_add_color(object->colors, fallback_color);
    da_add_color(object->colors, fallback_color);
    da_add_color(object->colors, fallback_color);

    return ptr;
}

char *skip_remaining_line(char *ptr) {
    while (*ptr) {
        // get first character in the next line
        if (*ptr == '\n') {
            return ptr + 1;
        }

        ++ptr;
    }

    return ptr;
}