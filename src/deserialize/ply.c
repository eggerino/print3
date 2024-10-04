#include "ply.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "util.h"

typedef enum Format {
    FORMAT_ASCII,
    FORMAT_BINARY_LITTLE_ENDIAN,
    FORMAT_BINARY_BIG_ENDIAN,
} Format;

typedef struct DataTypeInfo {
    uint8_t size;
    bool is_float;
    bool is_signed;
} DataTypeInfo;

typedef struct VertexProperty {
    int64_t binary_offset;
    int64_t index;
    DataTypeInfo data_type_info;
} VertexProperty;

typedef struct VertexElement {
    size_t count;
    size_t binary_size;
    size_t property_count;

    VertexProperty x;
    VertexProperty y;
    VertexProperty z;

    VertexProperty n_x;
    VertexProperty n_y;
    VertexProperty n_z;

    VertexProperty r;
    VertexProperty g;
    VertexProperty b;
    VertexProperty a;
} VertexElement;

typedef struct FaceElement {
    size_t count;

    DataTypeInfo count_info;
    DataTypeInfo item_info;
} FaceElement;

typedef struct Header {
    Format format;

    int64_t vertex_index;
    VertexElement vertex;

    int64_t face_index;
    FaceElement face;
} Header;

typedef struct Indices {
    size_t *items;
    size_t length;
    size_t capacity;
} Indices;

// Header parsing
static char *parse_header(char *ptr, Header *header);
static char *parse_vertex_properties(char *ptr, VertexElement *vertex);
static char *parse_face_properties(char *ptr, FaceElement *face);
static DataTypeInfo parse_data_type_info(char *ptr, char **peak);
static void str_cap(char *ptr, size_t n);

// Ascii data parsing
static char *ascii_parse_vertex(char *ptr, const VertexElement *element, Color fallback_color, Vertices *vertices,
                                Vertices *normals, Colors *colors);
static char *ascii_parse_face(char *ptr, const FaceElement *element, Indices *indices);

// Binary data parsing
static char *bin_parse_vertex(ByteOrdering ordering, char *ptr, const VertexElement *element, Color fallback_color,
                              Vertices *vertices, Vertices *normals, Colors *colors);
static char *bin_parse_face(ByteOrdering ordering, char *ptr, const FaceElement *element, Indices *indices);
static uint64_t bin_get_integer(void *buffer, ByteOrdering ordering, DataTypeInfo info);
static float bin_get_float(void *buffer, ByteOrdering ordering, DataTypeInfo info);

void ply_deserialize(void *buffer, size_t size, Color fallback_color, Scene *scene) {
    char *ptr = buffer;
    char *peak;

    Header header = {0};
    ptr = parse_header(ptr, &header);
    ByteOrdering ordering = header.format == FORMAT_BINARY_BIG_ENDIAN  // Value does not matter in ascii case
                                ? ORDERING_BIG_ENDIAN
                                : ORDERING_LITTLE_ENDIAN;

    Vertices vertices = {0};
    Vertices normals = {0};
    Colors colors = {0};
    Indices indices = {0};

    // Parse the data block
    for (int64_t element_index = 0; *ptr; ++element_index) {
        if (header.vertex_index == element_index) {
            if (header.format == FORMAT_ASCII) {
                ptr = ascii_parse_vertex(ptr, &header.vertex, fallback_color, &vertices, &normals, &colors);
            } else {
                ptr = bin_parse_vertex(ordering, ptr, &header.vertex, fallback_color, &vertices, &normals, &colors);
            }
        } else if (header.face_index == element_index) {
            if (header.format == FORMAT_ASCII) {
                ptr = ascii_parse_face(ptr, &header.face, &indices);
            } else {
                ptr = bin_parse_face(ordering, ptr, &header.face, &indices);
            }
        } else {
            ++ptr;
        }
    }

    // Try implementing the following algorithm to convert polygons to triangles
    // https://www.geometrictools.com/Documentation/TriangulationByEarClipping.pdf
    printf("TODO: Build scene out of polygons.\n");
    exit(1);
}

char *parse_header(char *ptr, Header *header) {
    char *peak;

    // Check magic number
    if (strncmp(ptr, "ply\n", 4)) {
        fprintf(stderr, "[ERR] Invalid format. Magic number \"ply\\n\" at start is missing.\n");
        exit(1);
    }
    ptr += 4;

    // Assert format is first info
    if (strncmp(ptr, "format ", 7)) {
        fprintf(stderr, "[ERR] Invalid format. Format specification must be on second line.\n");
        exit(1);
    }
    ptr += 7;

    // Parse format
    if (strncmp(ptr, "ascii ", 6) == 0) {
        header->format = FORMAT_ASCII;
        ptr += 6;
    } else if (strncmp(ptr, "binary_little_endian ", 21) == 0) {
        header->format = FORMAT_BINARY_LITTLE_ENDIAN;
        ptr += 21;
    } else if (strncmp(ptr, "binary_big_endian ", 18) == 0) {
        header->format = FORMAT_BINARY_BIG_ENDIAN;
        ptr += 18;
    } else {
        fprintf(stderr, "[ERR] Invalid format. Unknown format on second line.\n");
        exit(1);
    }

    // Assert version 1.0
    if (strncmp(ptr, "1.0\n", 4)) {
        fprintf(stderr, "[ERR] Invalid format. Only version 1.0 is supported.\n");
        exit(1);
    }
    ptr += 4;

    // Parse elements
    header->vertex_index = -1;  // Deactivate all element and activate them on visit
    header->face_index = -1;
    for (int element_index = 0; strncmp(ptr, "end_header\n", 11); ++element_index) {
        // Ignore comments
        if (strncmp(ptr, "comment ", 8) == 0) {
            element_index--;
            ptr = str_skip(ptr, "\n");
            if (!ptr) {
                fprintf(stderr, "[ERR] Invalid format. Comment must end with a new line.\n");
                exit(1);
            }
            continue;
        }

        // Assert the start with element keyword
        if (strncmp(ptr, "element ", 8)) {
            fprintf(stderr, "[ERR] Invalid format. Every element must start with the element keyword.\n");
        }
        ptr += 8;

        // Parse vertex element
        if (strncmp(ptr, "vertex ", 7) == 0) {
            ptr += 7;

            header->vertex_index = element_index;
            header->vertex.count = strtoll(ptr, &peak, 10);
            advance_or_err(ptr, peak, "[ERR] Invalid format. Vertex element is missing a count.\n");
            if (*ptr != '\n') {
                fprintf(stderr, "[ERR] Invalid format. Vertex count must be followed by new line.\n");
                exit(1);
            }
            ++ptr;

            ptr = parse_vertex_properties(ptr, &header->vertex);
        } else if (strncmp(ptr, "face ", 5) == 0) {
            ptr += 5;

            header->face_index = element_index;
            header->face.count = strtoll(ptr, &peak, 10);
            advance_or_err(ptr, peak, "[ERR] Invalid format. Face element is missing a count.\n");
            if (*ptr != '\n') {
                fprintf(stderr, "[ERR] Invalid format. Face count must be followed by new line.\n");
                exit(1);
            }
            ++ptr;

            ptr = parse_face_properties(ptr, &header->face);
        } else {
            str_cap(ptr, 40);
            fprintf(stderr, "[ERR] invalid format. Unknown element type -> element %s ... \n", ptr);
            exit(1);
        }
    }

    // Skip "end_header\n" header termination
    ptr += 11;

    return ptr;
}

char *parse_vertex_properties(char *ptr, VertexElement *vertex) {
    // Deactivate all properties, activate them on visit
    vertex->x.index = -1;
    vertex->y.index = -1;
    vertex->z.index = -1;
    vertex->n_x.index = -1;
    vertex->n_y.index = -1;
    vertex->n_z.index = -1;
    vertex->r.index = -1;
    vertex->g.index = -1;
    vertex->b.index = -1;
    vertex->a.index = -1;
    vertex->x.binary_offset = -1;
    vertex->y.binary_offset = -1;
    vertex->z.binary_offset = -1;
    vertex->n_x.binary_offset = -1;
    vertex->n_y.binary_offset = -1;
    vertex->n_z.binary_offset = -1;
    vertex->r.binary_offset = -1;
    vertex->g.binary_offset = -1;
    vertex->b.binary_offset = -1;
    vertex->a.binary_offset = -1;

    char *peak;
    size_t binary_offset = 0;
    size_t property_index = 0;
    for (; strncmp(ptr, "property ", 9) == 0 || strncmp(ptr, "comment ", 8) == 0; ++property_index) {
        // Ignore comments
        if (strncmp(ptr, "comment ", 8) == 0) {
            property_index--;
            ptr = str_skip(ptr, "\n");
            if (!ptr) {
                fprintf(stderr, "[ERR] Invalid format. Comment must end with a new line.\n");
                exit(1);
            }
            continue;
        }

        // Skip property keyword
        ptr += 9;

        VertexProperty property = {
            .index = property_index,
            .binary_offset = binary_offset,
            .data_type_info = parse_data_type_info(ptr, &peak),
        };
        advance_or_err(ptr, peak, "[ERR] Invalid format. Property does not have a valid datatype ->%s", ptr);

        // Aggregate the binary offset
        binary_offset += property.data_type_info.size;

        if (strncmp(ptr, "x\n", 2) == 0) {
            ptr += 2;
            vertex->x = property;
        } else if (strncmp(ptr, "y\n", 2) == 0) {
            ptr += 2;
            vertex->y = property;
        } else if (strncmp(ptr, "z\n", 2) == 0) {
            ptr += 2;
            vertex->z = property;
        } else if (strncmp(ptr, "nx\n", 3) == 0) {
            ptr += 3;
            vertex->n_x = property;
        } else if (strncmp(ptr, "ny\n", 3) == 0) {
            ptr += 3;
            vertex->n_y = property;
        } else if (strncmp(ptr, "nz\n", 3) == 0) {
            ptr += 3;
            vertex->n_z = property;
        } else if (strncmp(ptr, "red\n", 4) == 0) {
            ptr += 4;
            vertex->r = property;
        } else if (strncmp(ptr, "green\n", 6) == 0) {
            ptr += 6;
            vertex->g = property;
        } else if (strncmp(ptr, "blue\n", 5) == 0) {
            ptr += 5;
            vertex->b = property;
        } else if (strncmp(ptr, "alpha\n", 6) == 0) {
            ptr += 6;
            vertex->a = property;
        } else {
            // Ignore the property
            peak = str_skip(ptr, "\n");
            if (!peak) {
                str_cap(ptr, 20);
                fprintf(stderr, "[ERR] Invalid format. Ignored property does not have a newline at the end ->%s ...\n", ptr);
                exit(1);
            }
            ptr = peak;
        }
    }

    // Keep track of the total size per vertex
    vertex->binary_size = binary_offset;
    vertex->property_count = property_index;

    return ptr;
}

char *parse_face_properties(char *ptr, FaceElement *face) {
    char *peak;

    // Ignore comments
    while (strncmp(ptr, "comment ", 8) == 0) {
        ptr = str_skip(ptr, "\n");
        if (!ptr) {
            fprintf(stderr, "[ERR] Invalid format. Comment must end with a new line.\n");
            exit(1);
        }
    }

    // Assert the face only has a vertex_indices property (as list)
    if (strncmp(ptr, "property list ", 14)) {
        fprintf(stderr, "[ERR] Invalid format. Face element only supports list properties.\n");
        exit(1);
    }
    ptr += 14;

    face->count_info = parse_data_type_info(ptr, &peak);
    advance_or_err(ptr, peak, "[ERR] Invalid format. List property does not have a valid datatype for the count ->%s", ptr);

    face->item_info = parse_data_type_info(ptr, &peak);
    advance_or_err(ptr, peak, "[ERR] Invalid format. List property does not have a valid datatype for the items ->%s", ptr);

    if (strncmp(ptr, "vertex_indices\n", 15)) {
        fprintf(stderr, "[ERR] Invalid format. Face element only supports vertex indices as properties.\n");
        exit(1);
    }
    ptr += 15;

    return ptr;
}

#define DATA_TYPE_INFO_MAP_COUNT 16

struct {
    const char *identifier;
    DataTypeInfo info;
} data_type_info_map[DATA_TYPE_INFO_MAP_COUNT] = {
    {"char ", {.size = 1, .is_signed = true, .is_float = false}},
    {"int8 ", {.size = 1, .is_signed = true, .is_float = false}},
    {"uchar ", {.size = 1, .is_signed = false, .is_float = false}},
    {"uint8 ", {.size = 1, .is_signed = false, .is_float = false}},
    {"short ", {.size = 2, .is_signed = true, .is_float = false}},
    {"int16 ", {.size = 2, .is_signed = true, .is_float = false}},
    {"ushort ", {.size = 2, .is_signed = false, .is_float = false}},
    {"uint16 ", {.size = 2, .is_signed = false, .is_float = false}},
    {"int ", {.size = 4, .is_signed = true, .is_float = false}},
    {"int32 ", {.size = 4, .is_signed = true, .is_float = false}},
    {"uint ", {.size = 4, .is_signed = false, .is_float = false}},
    {"uint32 ", {.size = 4, .is_signed = false, .is_float = false}},
    {"float ", {.size = 4, .is_signed = true, .is_float = true}},
    {"float32 ", {.size = 4, .is_signed = true, .is_float = true}},
    {"double ", {.size = 8, .is_signed = true, .is_float = true}},
    {"float64 ", {.size = 8, .is_signed = true, .is_float = true}},
};

DataTypeInfo parse_data_type_info(char *ptr, char **peak) {
    for (int i = 0; i < DATA_TYPE_INFO_MAP_COUNT; ++i) {
        size_t n = strlen(data_type_info_map[i].identifier);

        if (strncmp(ptr, data_type_info_map[i].identifier, n) == 0) {
            *peak = ptr + n;
            return data_type_info_map[i].info;
        }
    }

    *peak = ptr;
    return (DataTypeInfo){0};
}

void str_cap(char *ptr, size_t n) {
    while (*ptr) {
        if (n == 0) {
            *ptr = 0;
            return;
        }
        ++ptr;
        --n;
    }
}

// ******************
// Ascii data parsing
// ******************

char *ascii_parse_vertex(char *ptr, const VertexElement *element, Color fallback_color, Vertices *vertices,
                         Vertices *normals, Colors *colors) {
    char *peak;
    bool use_fallback_color =
        element->r.index == -1 && element->g.index == -1 && element->b.index == -1 && element->a.index == -1;

    bool use_normals = element->n_x.index != -1 || element->n_y.index != -1 || element->n_z.index != -1;

    // Use fallback color when no color is given
    if (use_fallback_color) {
        for (size_t i = 0; i < element->count; ++i) {
            da_add_color(*colors, fallback_color);
        }
    }

    for (size_t vertex_index = 0; vertex_index < element->count; ++vertex_index) {
        Vector3 v = {0};
        Vector3 n = {0};
        Color c = {0, 0, 0, 255};

        for (size_t property_index = 0; property_index < element->property_count; ++property_index) {
#define read_float(prop, dest, msg)                       \
    if (property_index == (prop).index) {                 \
        (dest) = strtod(ptr, &peak);                      \
        advance_or_err(ptr, peak, msg, vertex_index + 1); \
        continue;                                         \
    }

#define read_integer(prop, dest, msg)                     \
    if (property_index == (prop).index) {                 \
        (dest) = strtoll(ptr, &peak, 10);                 \
        advance_or_err(ptr, peak, msg, vertex_index + 1); \
        continue;                                         \
    }

            read_float(element->x, v.x, "[ERR] Invalid format. %d-th vertex does not have a x component.\n");
            read_float(element->y, v.y, "[ERR] Invalid format. %d-th vertex does not have a y component.\n");
            read_float(element->z, v.z, "[ERR] Invalid format. %d-th vertex does not have a z component.\n");

            read_float(element->n_x, n.x, "[ERR] Invalid format. %d-th vertex's normal does not have a x component.\n");
            read_float(element->n_y, n.y, "[ERR] Invalid format. %d-th vertex's normal does not have a y component.\n");
            read_float(element->n_z, n.z, "[ERR] Invalid format. %d-th vertex's normal does not have a z component.\n");

            read_integer(element->r, c.r, "[ERR] Invalid format. %d-th vertex's color does not have a red component.\n");
            read_integer(element->g, c.g, "[ERR] Invalid format. %d-th vertex's color does not have a green component.\n");
            read_integer(element->b, c.b, "[ERR] Invalid format. %d-th vertex's color does not have a blue component.\n");
            read_integer(element->a, c.a, "[ERR] Invalid format. %d-th vertex's color does not have a alpha component.\n");

#undef read_float
#undef read_integer
        }

        da_add_vector3(*vertices, v);

        if (use_normals) {
            da_add_vector3(*normals, n);
        }

        if (!use_fallback_color) {
            da_add_color(*colors, c);
        }

        // Go to next line
        peak = str_skip(ptr, "\n");
        if (!peak) {
            while (*ptr) {
                ++ptr;
            }
        } else {
            ptr = peak;
        }
    }

    return ptr;
}

char *ascii_parse_face(char *ptr, const FaceElement *element, Indices *indices) {
    char *peak;

    for (size_t face_index = 0; face_index < element->count; ++face_index) {
        size_t index_count = strtoll(ptr, &peak, 10);
        advance_or_err(ptr, peak, "[ERR] Invalid format. %d-th face does not have a vertex count.\n", face_index + 1);
        da_add(*indices, index_count);

        for (size_t index_index = 0; index_index < index_count; ++index_index) {
            size_t index = strtoll(ptr, &peak, 10);
            advance_or_err(ptr, peak, "[ERR] Invalid format. %d-th face does not have a %d-th vertex index.\n",
                           face_index + 1, index_index + 1);
            da_add(*indices, index);
        }

        // Go to next line
        peak = str_skip(ptr, "\n");
        if (!peak) {
            while (*ptr) {
                ++ptr;
            }
        } else {
            ptr = peak;
        }
    }

    return ptr;
}

// *******************
// Binary data parsing
// *******************

char *bin_parse_vertex(ByteOrdering ordering, char *ptr, const VertexElement *element, Color fallback_color,
                       Vertices *vertices, Vertices *normals, Colors *colors) {
    bool use_fallback_color =
        element->r.index == -1 && element->g.index == -1 && element->b.index == -1 && element->a.index == -1;

    bool use_normals = element->n_x.index != -1 || element->n_y.index != -1 || element->n_z.index != -1;

    // Use fallback color when no color is given
    if (use_fallback_color) {
        for (size_t i = 0; i < element->count; ++i) {
            da_add_color(*colors, fallback_color);
        }
    }

    for (size_t vertex_index = 0; vertex_index < element->count; ++vertex_index) {
        Vector3 v = {0};
        Vector3 n = {0};
        Color c = {0, 0, 0, 255};

#define read_float(prop, dest)                                                               \
    if ((prop).index != -1) {                                                                \
        (dest) = bin_get_float(ptr + (prop).binary_offset, ordering, (prop).data_type_info); \
    }
#define read_integer(prop, dest)                                                               \
    if ((prop).index != -1) {                                                                  \
        (dest) = bin_get_integer(ptr + (prop).binary_offset, ordering, (prop).data_type_info); \
    }

        read_float(element->x, v.x);
        read_float(element->y, v.y);
        read_float(element->z, v.z);

        read_float(element->n_x, n.x);
        read_float(element->n_y, n.y);
        read_float(element->n_z, n.z);

        read_integer(element->r, c.r);
        read_integer(element->g, c.g);
        read_integer(element->b, c.b);
        read_integer(element->a, c.a);

        da_add_vector3(*vertices, v);

        if (use_normals) {
            da_add_vector3(*normals, n);
        }

        if (!use_fallback_color) {
            da_add_color(*colors, c);
        }

        // Go to next vertex
        ptr += element->binary_size;
    }

    return ptr;
}

char *bin_parse_face(ByteOrdering ordering, char *ptr, const FaceElement *element, Indices *indices) {
    // Counts and indices are always non negative every number can be safely reinterpreted as unsigned
    for (size_t face_index = 0; face_index < element->count; ++face_index) {
        size_t index_count = bin_get_integer(ptr, ordering, element->count_info);
        ptr += element->count_info.size;
        da_add(*indices, index_count);

        for (size_t index_index = 0; index_index < index_count; ++index_index) {
            size_t index = bin_get_integer(ptr, ordering, element->item_info);
            ptr += element->item_info.size;
            da_add(*indices, index);
        }
    }

    return ptr;
}

uint64_t bin_get_integer(void *buffer, ByteOrdering ordering, DataTypeInfo info) {
    if (info.size == 1) {
        return binary_buffer_to_u8(buffer);
    }

    if (info.size == 2) {
        return binary_buffer_to_u16(buffer, ordering);
    }

    return binary_buffer_to_u32(buffer, ordering);
}

float bin_get_float(void *buffer, ByteOrdering ordering, DataTypeInfo info) {
    return info.size == 4 ? binary_buffer_to_f32_IEEE754(buffer, ordering) : binary_buffer_to_f64_IEEE754(buffer, ordering);
}
