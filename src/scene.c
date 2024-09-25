#include "scene.h"

void scene_free_members(Scene *scene) {
    for (size_t i = 0; i < scene->objects.length; ++i) {
        free(scene->objects.items[i].colors.items);
        free(scene->objects.items[i].vertices.items);
    }

    free(scene->objects.items);
}

void scene_add_demo_object(Scene *scene) {
    // Add a pyramid shaped object to the scene
    Vector3 vertices[5] = {(Vector3){1, 1, 0}, (Vector3){-1, 1, 0}, (Vector3){-1, -1, 0}, (Vector3){1, -1, 0},
                           (Vector3){0, 0, 1}};
    Color colors[4] = {RED, GREEN, BLUE, YELLOW};
    int indices[12] = {0, 1, 4, 1, 2, 4, 2, 3, 4, 3, 0, 4};

    Object obj = {0};

    for (int i = 0; i < 4; ++i) {
        Color c = colors[i];
        da_add_color(obj.colors, c);
        da_add_color(obj.colors, c);
        da_add_color(obj.colors, c);

        Vector3 v1 = vertices[indices[3 * i]];
        Vector3 v2 = vertices[indices[3 * i + 1]];
        Vector3 v3 = vertices[indices[3 * i + 2]];
        da_add_vector3(obj.vertices, v1);
        da_add_vector3(obj.vertices, v2);
        da_add_vector3(obj.vertices, v3);
    }

    da_add(scene->objects, obj);
}
