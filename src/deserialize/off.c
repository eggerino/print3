#include "off.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "raymath.h"
#include "util.h"

typedef struct Header {
    bool use_texture_coordinates;    // "ST" flag
    bool use_colors;                 // "C" flag
    bool use_normals;                // "N" flag
    bool use_homogeneous_component;  // "4" flag
    bool use_n_dimensions;           // "n" flag
    long n_dimensions;               // only present when use_n_dimension is true
    long n_vertices;
    long n_faces;
    long n_edges;
} Header;

typedef struct Normals {
    float *items;
    size_t length;
    size_t capacity;
} Normals;

#define advance_err(...)                  \
    do {                                  \
        if (peak == ptr) {                \
            fprintf(stderr, __VA_ARGS__); \
            exit(1);                      \
        }                                 \
        ptr = peak;                       \
    } while (0)

#define vector3_at(items, index) \
    (Vector3) { (items)[3 * (index)], (items)[3 * (index) + 1], (items)[3 * (index) + 2] }

#define color_at(items, index) \
    (Color) { (items)[4 * (index)], (items)[4 * (index) + 1], (items)[4 * (index) + 2], (items)[4 * (index) + 3] }

static char *parse_header(char *ptr, Header *header);
static char *parse_vertices(char *ptr, const Header *header, Vertices *vertices, Colors *colors, Normals *normals);
static char *parse_faces(char *ptr, Color fallback_color, const Header *header, const Vertices *vertices,
                         const Colors *colors, const Normals *normals, Object *object);
static char *next_token(char *ptr);
static bool has_numeric_in_line(const char *ptr);
static bool has_float_in_line(const char *ptr);

void off_deserialize(void *buffer, size_t size, Color fallback_color, Scene *scene) {
    char *ptr = buffer;

    Header header = {0};
    ptr = parse_header(ptr, &header);
    if (header.use_n_dimensions && header.n_dimensions >= 4) {
        fprintf(stderr, "[ERR] Unsupported. nOff with n = %d is given. Only n < 4 is supported.\n");
        exit(1);
    }

    Vertices vertices = {0};
    Colors colors = {0};
    Normals normals = {0};
    ptr = parse_vertices(ptr, &header, &vertices, &colors, &normals);

    Object object = {0};
    ptr = parse_faces(ptr, fallback_color, &header, &vertices, &colors, &normals, &object);

    if (*ptr) {
        fprintf(stderr, "[ERR] Invalid format. Expected end of file after the %d-th face.\n", header.n_faces);
        exit(1);
    }

    da_add(scene->objects, object);
}

char *parse_header(char *ptr, Header *header) {
    bool has_header = isupper(*ptr) || islower(*ptr) || (*ptr == '4' && (ptr[1] == 'n' || ptr[1] == 'O'));
    if (!has_header) {
        return next_token(ptr);
    }

    if (strncmp(ptr, "ST", 2) == 0) {
        header->use_texture_coordinates = true;
        ptr += 2;
    }

    if (*ptr == 'C') {
        header->use_colors = true;
        ++ptr;
    }

    if (*ptr == 'N') {
        header->use_normals = true;
        ++ptr;
    }

    if (*ptr == '4') {
        header->use_homogeneous_component = true;
        ++ptr;
    }

    if (*ptr == 'n') {
        header->use_n_dimensions = true;
        ++ptr;
    }

    if (strncmp(ptr, "OFF", 3) != 0) {
        fprintf(stderr, "[ERR] Invalid format. No OFF keyword found in header line.\n");
        exit(1);
    }
    ptr += 3;

    ptr = next_token(ptr);

    char *peak;
    if (header->use_n_dimensions) {
        header->n_dimensions = strtol(ptr, &peak, 10);
        advance_err("[ERR] Invalid format. nOFF needs a dimension as first value.\n");
        ptr = next_token(peak);
    }

    header->n_vertices = strtol(ptr, &peak, 10);
    advance_err("[ERR] Invalid format. No vertex count.\n");
    ptr = next_token(ptr);

    header->n_faces = strtol(ptr, &peak, 10);
    advance_err("[ERR] Invalid format. No face count.\n");
    ptr = next_token(ptr);

    header->n_edges = strtol(ptr, &peak, 10);
    advance_err("[ERR] Invalid format. No edge count.\n");
    ptr = next_token(ptr);

    return ptr;
}

#define parse_vector3_err(da, dims, ...)                                                            \
    do {                                                                                            \
        for (size_t parse_vector3_err_i = 0; parse_vector3_err_i < (dims); ++parse_vector3_err_i) { \
            da_add(da, strtof(ptr, &peak));                                                         \
            advance_err(__VA_ARGS__);                                                               \
        }                                                                                           \
        for (size_t parse_vector3_err_i = (dims); parse_vector3_err_i < 3; ++parse_vector3_err_i) { \
            da_add(da, 0.0f);                                                                       \
        }                                                                                           \
    } while (0)

char *parse_vertices(char *ptr, const Header *header, Vertices *vertices, Colors *colors, Normals *normals) {
    char *peak;
    size_t vertex_dimensions = header->use_n_dimensions ? header->n_dimensions : 3;

    for (size_t i_vertex = 0; i_vertex < header->n_vertices; ++i_vertex) {
        // Parse the vertex coordinates
        for (size_t i_dimension = 0; i_dimension < vertex_dimensions; ++i_dimension) {
            float vertex_component = strtof(ptr, &peak);
            advance_err("[ERR] Invalid format. %d-th component of the %d-th vertex must be a floating point number.\n",
                        i_dimension + 1, i_vertex + 1);

            da_add(*vertices, vertex_component);
        }
        for (size_t i = vertex_dimensions; i < 3; ++i) da_add(*vertices, 0.0f);  // Pad with zeros

        // Transform homogenous coordinates to cathesian coordinates if needed
        if (header->use_homogeneous_component) {
            float homogeneous_component = strtof(ptr, &peak);
            advance_err(
                "[ERR] Invalid format. Homongeneous component of the %d-th vertex must be a floating point number.\n",
                i_vertex + 1);

            for (size_t i_dimension = 0; i_dimension < 3; ++i_dimension) {
                vertices->items[vertices->length - 1 - i_dimension] /= homogeneous_component;
            }
        }

        // parse normals if present
        if (header->use_normals) {
            for (size_t i_dimension = 0; i_dimension < vertex_dimensions; ++i_dimension) {
                float normal_component = strtof(ptr, &peak);
                advance_err(
                    "[ERR] Invalid format. %d-th component of the %d-th vertex's normal must be a floating point number.\n",
                    i_dimension + 1, i_vertex + 1);

                da_add(*normals, normal_component);
            }

            // Pad with zeros
            for (size_t i = vertex_dimensions; i < 3; ++i) da_add(*normals, 0.0f);
        }

        // Parse colors if present
        if (header->use_colors) {
            for (size_t i_color = 0; i_color < 4; ++i_color) {
                unsigned char color_component = strtol(ptr, &peak, 10);
                advance_err("[ERR] Invalid format. %d-th color component of the %d-th vertex must be a byte.\n", i_color + 1,
                            i_vertex + 1);

                da_add(*colors, color_component);
            }
        }

        // Ignore possible texture coordinates
        peak = str_skip(ptr, "\n");
        if (!peak) {
            fprintf(stderr, "[ERR] Invalid format. End of file is too early. %d vertices expected but only %d found.\n",
                    header->n_vertices, i_vertex + 1);
            exit(1);
        }
        ptr = next_token(peak);
    }

    return ptr;
}

char *parse_faces(char *ptr, Color fallback_color, const Header *header, const Vertices *vertices, const Colors *colors,
                  const Normals *normals, Object *object) {
    char *peak;
    for (size_t i_face = 0; i_face < header->n_faces; ++i_face) {
        long number_vertices = strtol(ptr, &peak, 10);
        advance_err("[ERR] Invalid format. %d-th face has no vertex count.\n", i_face + 1);

        long *vertex_indices = malloc(number_vertices * sizeof(long));
        for (long i_vertex = 0; i_vertex < number_vertices; ++i_vertex) {
            vertex_indices[i_vertex] = strtol(ptr, &peak, 10);
            advance_err("[ERR] Invalid format. %d-th face has no %d-thvertex.\n", i_face + 1, i_vertex + 1);
        }

        bool has_face_color = false;
        unsigned char face_color[4] = {255, 255, 255, 255};
        if (has_numeric_in_line(ptr)) {
            has_face_color = true;

            if (has_float_in_line(ptr)) {
                for (size_t i_color = 0; i_color < 3; ++i_color) {
                    face_color[i_color] = (unsigned char)(strtof(ptr, &peak) * 255.0);
                    advance_err("[ERR] Invalid format. %d-th face has no %d-th color component.\n", i_face + 1, i_color + 1);
                }

                if (has_numeric_in_line(ptr)) {
                    face_color[3] = (unsigned char)(strtof(ptr, &peak) * 255.0);
                    advance_err("[ERR] Invalid format. %d-th face has no alpha color component.\n", i_face + 1);
                }
            } else {
                for (size_t i_color = 0; i_color < 3; ++i_color) {
                    face_color[i_color] = (unsigned char)strtol(ptr, &peak, 10);
                    advance_err("[ERR] Invalid format. %d-th face has no %d-th color component.\n", i_face + 1, i_color + 1);
                }

                if (has_numeric_in_line(ptr)) {
                    face_color[3] = (unsigned char)strtol(ptr, &peak, 10);
                    advance_err("[ERR] Invalid format. %d-th face has no alpha color component.\n", i_face + 1);
                }
            }
        }

        for (long i_vertex = 0; i_vertex + 2 < number_vertices; ++i_vertex) {
            Vector3 v1 = vector3_at(vertices->items, vertex_indices[i_vertex]);
            Vector3 v2 = vector3_at(vertices->items, vertex_indices[i_vertex + 1]);
            Vector3 v3 = vector3_at(vertices->items, vertex_indices[i_vertex + 2]);

            bool shuffled = false;
            if (header->use_normals) {
                Vector3 n1 = vector3_at(normals->items, vertex_indices[i_vertex]);
                Vector3 n2 = vector3_at(normals->items, vertex_indices[i_vertex + 1]);
                Vector3 n3 = vector3_at(normals->items, vertex_indices[i_vertex + 2]);

                Vector3 normal = Vector3Scale(Vector3Add(n1, Vector3Add(n2, n3)), 1.0f / 3.0f);
                shuffled = order_vertices(&normal, &v1, &v2, &v3);
            } else {
                shuffled = i_vertex % 2;
                if (shuffled) {
                    Vector3 temp = v2;
                    v2 = v3;
                    v3 = temp;
                }
            }

            da_add_vector3(object->vertices, v1);
            da_add_vector3(object->vertices, v2);
            da_add_vector3(object->vertices, v3);

            if (has_face_color) {
                da_add4(object->colors, face_color[0], face_color[1], face_color[2], face_color[3]);
                da_add4(object->colors, face_color[0], face_color[1], face_color[2], face_color[3]);
                da_add4(object->colors, face_color[0], face_color[1], face_color[2], face_color[3]);
            } else if (header->use_colors) {
                Color c1 = color_at(colors->items, vertex_indices[i_vertex]);
                Color c2 = shuffled ? color_at(colors->items, vertex_indices[i_vertex + 1])
                                    : color_at(colors->items, vertex_indices[i_vertex + 2]);
                Color c3 = shuffled ? color_at(colors->items, vertex_indices[i_vertex + 2])
                                    : color_at(colors->items, vertex_indices[i_vertex + 1]);

                da_add_color(object->colors, c1);
                da_add_color(object->colors, c2);
                da_add_color(object->colors, c3);
            } else {
                da_add_color(object->colors, fallback_color);
                da_add_color(object->colors, fallback_color);
                da_add_color(object->colors, fallback_color);
            }
        }

        free(vertex_indices);
        ptr = next_token(ptr);
    }
    return ptr;
}

char *next_token(char *ptr) {
    ptr = str_skip_whitespace(ptr);

    // Skip comments
    while (*ptr == '#') {
        char *peak = str_skip(ptr, "\n");
        if (!peak) {  // Comment reaches end of file
            while (*(++ptr)) {
            }
            return ptr;
        }

        ptr = str_skip_whitespace(peak);
    }

    return ptr;
}

bool has_numeric_in_line(const char *ptr) {
    for (; *ptr; ++ptr) {
        if (*ptr == '\n') return false;
        if (*ptr == '#') return false;
        if ('0' <= *ptr && *ptr <= '9') return true;
    }
    return false;
}

bool has_float_in_line(const char *ptr) {
    for (; *ptr; ++ptr) {
        if (*ptr == '\n') return false;
        if (*ptr == '#') return false;
        if (*ptr == '.') return true;
    }
    return false;
}
