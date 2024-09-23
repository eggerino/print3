#include "stdin.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 1024

static Color read_color(char *buffer, char **end);
static Vector3 read_vector3(char *buffer, char **end);

void stdin_add_to_scene(FILE *stream, Scene *scene) {
    char buffer[BUFFER_SIZE];

    Object object = {0};

    // Read color first
    if (!fgets(buffer, BUFFER_SIZE, stream)) {
        fprintf(stderr, "Could not read color line.\n");
        exit(1);
    }
    Color color = read_color(buffer, NULL);

    // Read vertices until the "end" is given or the stream ends
    while (fgets(buffer, BUFFER_SIZE, stream)) {
        if (strncmp(buffer, "end", 3) == 0) {
            break;
        }

        char *current = buffer;
        char *peak;
        for (int i = 0; i < 3; ++i) {
            Vector3 vec = read_vector3(current, &peak);
            current = peak;
            da_add(scene->vertices, vec);
            da_add(object.surface, scene->vertices.length - 1);
        }
        da_add(object.colors, color);
    }

    da_add(scene->objects, object);
}

Color read_color(char *buffer, char **end) {
    char color[4];
    char *peak;

    for (int i = 0; i < 4; ++i) {
        color[i] = strtoul(buffer, &peak, 10);

        if (peak == buffer) {
            fprintf(stderr, "[ERR] Expected a number [0 - 255] for the %d. color component at ->%s within the buffer.\n",
                    i + 1, buffer);
            exit(1);
        }
        buffer = peak;
    }

    if (end) {
        *end = buffer;
    }

    return (Color){color[0], color[1], color[2], color[3]};
}

Vector3 read_vector3(char *buffer, char **end) {
    float vec[3];
    char *peak;

    for (int i = 0; i < 3; ++i) {
        vec[i] = strtof(buffer, &peak);

        if (peak == buffer) {
            fprintf(
                stderr,
                "[ERR] Expected a floating point number [REAL32] for the %d. vector component at ->%s within the buffer.\n",
                i + 1, buffer);
            exit(1);
        }
        buffer = peak;
    }

    if (end) {
        *end = buffer;
    }

    return (Vector3){vec[0], vec[1], vec[2]};
}
