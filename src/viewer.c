#include "viewer.h"

#include "raylib.h"

#define TARGET_FPS 60

static void draw_scene_3D(const ViewerOptions *opt, const Scene *scene);

void viewer_run(const ViewerOptions *opt, const Scene *scene, const bool *should_run) {
    InitWindow(opt->window_width, opt->window_height, opt->window_title);

    Camera camera = {0};
    camera.position = (Vector3){-3, -3, 0};
    camera.up.z = 1;
    camera.fovy = 45.0f;
    camera.projection = CAMERA_ORTHOGRAPHIC;

    SetTargetFPS(TARGET_FPS);

    // Check for ending signal from cancelation token and GUI events
    while (*should_run && !WindowShouldClose()) {
        // TODO: Handle user input to manipulate the camera

        BeginDrawing();

        ClearBackground(opt->background);

        BeginMode3D(camera);
        draw_scene_3D(opt, scene);
        EndMode3D();

        // TODO: Render a toggleable 2D pane with controls

        // TODO: Render a 3D coordinate system in the corner

        EndDrawing();
    }

    CloseWindow();
}

void draw_scene_3D(const ViewerOptions *opt, const Scene *scene) {
    for (size_t i_obj = 0; i_obj < scene->objects.length; ++i_obj) {
        Object obj = scene->objects.items[i_obj];
        for (size_t i_sur = 0; i_sur + 2 < obj.surface.length; i_sur += 3) {
            DrawTriangle3D(scene->vertices.items[obj.surface.items[i_sur]],
                           scene->vertices.items[obj.surface.items[i_sur + 1]],
                           scene->vertices.items[obj.surface.items[i_sur + 2]], obj.color);
        }
    }

    if (!opt->render_facets_both_sides) return;

    // Run the same loop again but swap the second and third vertex of each surface to achieve different implicit normal
    // vectors, which are used by raylib to determine if the surface is facing towards the camera
    for (size_t i_obj = 0; i_obj < scene->objects.length; ++i_obj) {
        Object obj = scene->objects.items[i_obj];
        for (size_t i_sur = 0; i_sur + 2 < obj.surface.length; i_sur += 3) {
            DrawTriangle3D(scene->vertices.items[obj.surface.items[i_sur]],
                           scene->vertices.items[obj.surface.items[i_sur + 2]],
                           scene->vertices.items[obj.surface.items[i_sur + 1]], obj.color);
        }
    }
}
