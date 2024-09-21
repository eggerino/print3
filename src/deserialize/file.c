#include "file.h"

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

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

void file_add_to_scene(const char *filename, Scene *scene) {
    MemoryDeserializer deserializer = get_deserializer(filename);

    // Open the file
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "[ERR] Could open file \"%s\".\n", filename);
        exit(1);
    }

    // Get the file size
    struct stat file_stat;
    if (fstat(fd, &file_stat)) {
        fprintf(stderr, "[ERR] Could not read form file \"%s\".\n", filename);
        exit(1);
    }
    size_t size = file_stat.st_size;

    // Map the file
    void *buffer = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (buffer == MAP_FAILED) {
        fprintf(stderr, "[ERR] Could not map the file \"%s\".\n", filename);
        exit(1);
    }

    // Dispatch the deserializer
    deserializer(buffer, size, scene);

    // Unmap the file
    if (munmap(buffer, size)) {
        fprintf(stderr, "[ERR] Could not unmap the file \"%s\".\n", filename);
        exit(1);
    }

    // Close file descriptor
    close(fd);
}

MemoryDeserializer get_deserializer(const char *filename) {
    const char *extension = strrchr(filename, '.');
    for (size_t i = 0; i < EXTENSION_MAP_COUNT; ++i) {
        if (strcmp(extension, extension_map[i].extension) == 0) {
            return extension_map[i].deserializer;
        }
    }

    fprintf(stderr, "[ERR] File extension \"%s\" of file \"%s\" is not supported.\n", extension, filename);
    exit(1);
}
