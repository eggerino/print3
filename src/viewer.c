#include "viewer.h"

#include <math.h>

#include "raylib.h"
#include "raymath.h"

#define TARGET_FPS 60
#define MAX_SCENE_RADIUS 1000.0

#define ROTATION_SENSITIVITY 1e-3
#define PAN_SENEITIVITY 5e-3
#define ZOOM_SENSITIVITY 0.3

static void draw_scene_3D(const ViewerOptions *opt, const Scene *scene);
static void reset_camera(Camera *camera);
static void update_camera(Camera *camera);

void viewer_run(const ViewerOptions *opt, const Scene *scene, const bool *should_run) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(opt->initial_window_width, opt->initial_window_height, opt->window_title);

    Camera camera = {0};
    reset_camera(&camera);

    SetTargetFPS(TARGET_FPS);

    // Check for ending signal from cancelation token and GUI events
    while (*should_run && !WindowShouldClose()) {
        update_camera(&camera);

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

void reset_camera(Camera *camera) {
    camera->position = (Vector3){-MAX_SCENE_RADIUS, 0, 0};
    camera->target = (Vector3){0, 0, 0};
    camera->up = (Vector3){0, 0, 1};
    camera->fovy = 45.0f;
    camera->projection = CAMERA_ORTHOGRAPHIC;
}

void update_camera(Camera *camera) {
    // Rotate via left mouse button down + drag
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        Vector2 mouse_delta = GetMouseDelta();

        // Transform camera orientation representation from (pos, tar, up) -> (view, up, right)
        Vector3 view = Vector3Subtract(camera->target, camera->position);
        Vector3 up = camera->up;
        Vector3 right = Vector3CrossProduct(view, up);

        // Handle up - down (rotation along right)
        float up_down_angle = ROTATION_SENSITIVITY * mouse_delta.y;
        view = Vector3RotateByAxisAngle(view, right, up_down_angle);
        up = Vector3RotateByAxisAngle(up, right, up_down_angle);

        // Handle left - right (rotation along up)
        float left_right_angle = ROTATION_SENSITIVITY * mouse_delta.x;
        view = Vector3RotateByAxisAngle(view, up, left_right_angle);

        // Transform back while keeping target fixed
        camera->up = up;
        camera->position = Vector3Subtract(camera->target, view);
    }

    // Pan via right mouse down + drag
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        Vector2 mouse_delta = GetMouseDelta();

        // Get unit vectors along the cameras up and right direction
        Vector3 view = Vector3Subtract(camera->target, camera->position);
        Vector3 up_unit = Vector3Normalize(camera->up);
        Vector3 right_unit = Vector3Normalize(Vector3CrossProduct(view, up_unit));

        // Get relative pan vectors
        float up_distance = PAN_SENEITIVITY * mouse_delta.y;
        Vector3 up_pan = Vector3Scale(up_unit, up_distance);

        float right_distance = -PAN_SENEITIVITY * mouse_delta.x;
        Vector3 right_pan = Vector3Scale(right_unit, right_distance);

        Vector3 pan = Vector3Add(up_pan, right_pan);

        // Apply pan tranlation to camera position and target
        camera->position = Vector3Add(camera->position, pan);
        camera->target = Vector3Add(camera->target, pan);
    }

    // Zooming via mouse wheel
    float mouse_wheel_move = GetMouseWheelMove();
    float zoom_factor = powf(1.0f + ZOOM_SENSITIVITY, mouse_wheel_move);
    camera->fovy *= zoom_factor;

    // Reset camera via pressing "R"
    if (IsKeyDown(KEY_R)) {
        reset_camera(camera);
    }
}
