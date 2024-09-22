#include "file.h"

#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "stl.h"

#define EXTENSION_MAP_COUNT 1

// Use array of key value pair and scan linearly for the entry
// With many deserializers consider implementing a hash table
struct {
    const char *extension;
    MemoryDeserializer deserializer;
} extension_map[EXTENSION_MAP_COUNT] = {{".stl", stl_deserialize}};

static MemoryDeserializer get_deserializer(const char *filename);

void file_add_to_scene(const char *filename, Color fallback_color, Scene *scene) {
    MemoryDeserializer deserializer = get_deserializer(filename);

    // Open the file
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "[ERR] Could not open file \"%s\".\n", filename);
        exit(1);
    }

    // Get the file size
    if (fseek(fp, 0, SEEK_END)) {
        fprintf(stderr, "[ERR] Could seek end of file \"%s\".\n", filename);
        exit(1);
    }
    size_t size = ftell(fp);
    if (fseek(fp, 0, SEEK_SET)) {
        fprintf(stderr, "[ERR] Could seek beginning of file \"%s\".\n", filename);
        exit(1);
    }

    // Create a buffer for the content + a null termination character.
    static_assert(sizeof(char) == 1);
    char *buffer = malloc(size + 1);
    if (!buffer) {
        fprintf(stderr, "[Err] Could not allocate a buffer of size %u.\n", size + 1);
        exit(1);
    }

    // Read the file into the buffer and add the null termination character.
    if (fread(buffer, 1, size, fp) != size) {
        fprintf(stderr, "[Err] Could read the content of file \"%s\".\n", filename);
        exit(1);
    }
    buffer[size] = '\0';

    // Dispatch the deserializer
    deserializer(buffer, size, fallback_color, scene);

    // Free resources
    free(buffer);
    fclose(fp);
}

MemoryDeserializer get_deserializer(const char *filename) {
    const char *extension = strrchr(filename, '.');
    extension = extension ? extension : filename;  // use filename as extension if no dot is found

    for (size_t i = 0; i < EXTENSION_MAP_COUNT; ++i) {
        if (strcmp(extension, extension_map[i].extension) == 0) {
            return extension_map[i].deserializer;
        }
    }

    fprintf(stderr, "[ERR] File extension \"%s\" of file \"%s\" is not supported.\n", extension, filename);
    exit(1);
}
