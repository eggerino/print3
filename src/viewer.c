#include "viewer.h"

#include <math.h>

#include "raylib.h"
#include "raymath.h"

#define TARGET_FPS 60
#define MAX_SCENE_RADIUS 1000.0

#define ROTATION_SENSITIVITY 1e-3
#define PAN_SENEITIVITY 5e-3
#define ZOOM_SENSITIVITY 0.3

typedef struct ViewerContext {
    bool display_hud;
} ViewerContext;

static void draw_scene_3D(const ViewerOptions *opt, const Scene *scene);

static void reset_camera(Camera *camera);
static void update_camera(Camera *camera);

static void update_context(ViewerContext *context);

static void draw_control_info(const ViewerOptions *opt, const ViewerContext *context);
static void draw_fps(const ViewerContext *context);

void viewer_run(const ViewerOptions *opt, const Scene *scene, const bool *should_run) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(opt->initial_window_width, opt->initial_window_height, opt->window_title);

    Camera camera = {0};
    reset_camera(&camera);

    ViewerContext context = {0};

    SetTargetFPS(TARGET_FPS);

    // Check for ending signal from cancelation token and GUI events
    while (*should_run && !WindowShouldClose()) {
        update_camera(&camera);
        update_context(&context);

        BeginDrawing();

        ClearBackground(opt->background);

        BeginMode3D(camera);
        draw_scene_3D(opt, scene);
        EndMode3D();

        draw_control_info(opt, &context);
        draw_fps(&context);

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

void update_context(ViewerContext *context) {
    // Toggle hub by pressing "H"
    if (IsKeyPressed(KEY_H)) {
        context->display_hud = !context->display_hud;
    }
}

#define draw_control_entry(binding, description)                              \
    do {                                                                      \
        DrawRectangle(8, 8 + 16 * entry, 330, 16, colors[(entry + 1) % 2]);   \
        DrawText((binding), 10, 10 + 16 * entry, 12, colors[entry % 2]);      \
        DrawText((description), 250, 10 + 16 * entry, 12, colors[entry % 2]); \
        ++entry;                                                              \
    } while (0)

void draw_control_info(const ViewerOptions *opt, const ViewerContext *context) {
    Color colors[2] = {DARKGRAY, LIGHTGRAY};
    size_t entry = 0;

    if (!context->display_hud) {
        // show a hint to tofggle hud for better ux
        // This behaviour is controlable by the true no hud option
        if (!opt->true_no_hud) {
            // show a hint to on how to toggle hut
            draw_control_entry("H", "Toggle HUD");
        }
        return;
    }

    draw_control_entry("H", "Toggle HUD");
    draw_control_entry("R", "Reset camera");
    draw_control_entry("Left Mouse Button + Mouse Drag", "Rotate");
    draw_control_entry("Right Mouse Button + Mouse Drag", "Pan");
    draw_control_entry("Mouse Wheel", "Zoom");
}

void draw_fps(const ViewerContext *context) {
    if (!context->display_hud) return;

    int scree_width = GetScreenWidth();
    DrawFPS(scree_width - 100, 10);
}