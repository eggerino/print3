#include "scene.h"

void scene_free_members(Scene *scene) {
    free(scene->vertices.items);

    for (size_t i = 0; i < scene->objects.length; ++i) {
        free(scene->objects.items[i].surface.items);
    }
    free(scene->objects.items);
}

void scene_add_demo_object(Scene *scene) {
    // Add a pyramid shaped object to the scene
    da_add(scene->vertices, ((Vector3){1, 1, 0}));
    da_add(scene->vertices, ((Vector3){-1, 1, 0}));
    da_add(scene->vertices, ((Vector3){-1, -1, 0}));
    da_add(scene->vertices, ((Vector3){1, -1, 0}));
    da_add(scene->vertices, ((Vector3){0, 0, 1}));

    size_t tip = scene->vertices.length - 1;

    Object obj = {0};
    obj.color = BLUE;

    da_add3(obj.surface, tip - 4, tip - 3, tip);
    da_add3(obj.surface, tip - 3, tip - 2, tip);
    da_add3(obj.surface, tip - 2, tip - 1, tip);
    da_add3(obj.surface, tip - 1, tip - 4, tip);

    da_add(scene->objects, obj);
}
