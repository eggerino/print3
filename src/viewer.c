#include "viewer.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "raylib.h"
#include "raymath.h"

#define TARGET_FPS 60

#define COS_VIEW_WIDTH 200
#define COS_VIEW_HEIGHT 200

#define ROTATION_SENSITIVITY 1e-3
#define PAN_SENEITIVITY 1.8
#define ZOOM_SENSITIVITY 0.3

typedef struct SurfaceSelection {
    bool active;
    Vector3 point;
    Vector3 normal;
    Vector3 vertices[3];
} SurfaceSelection;

typedef struct ViewerContext {
    const ViewerOptions *options;
    float scene_radius;
    bool display_hud;
    bool display_cos;
    SurfaceSelection surface_selection;
} ViewerContext;

// Scene
static float get_scene_radius(const Scene *scene);
static void set_empty_mesh_with_scene(const Scene *scene, Mesh *mesh);
static void invert_normals_mesh(Mesh *mesh);
static void set_mesh_color(Color color, Mesh *mesh);
static void draw_surface_selection(const ViewerContext *context);
static void create_screenshot();

// Viewer context
static void update_context(const Scene *scene, const Camera *camera, ViewerContext *context);
static void set_surface_selection(const Scene *scene, const Camera *camera, SurfaceSelection *surface_selection);

// Camera control
static void reset_camera(const ViewerContext *context, Camera *camera);
static void update_camera(const ViewerContext *context, Camera *camera);

// HUD
static void draw_control_info(const ViewerContext *context);
static void draw_fps(const ViewerContext *context);

// Visualization of the coordinate system
static void draw_arrow(Vector3 start, Vector3 dir_normalized, float line_length, float line_radius, float tip_length,
                       float tip_radius, int sides, Color color);
static void render_cos_view(const ViewerContext *context, Camera *camera, RenderTexture *cos_view);
static void draw_rendered_cos_view(const ViewerContext *context, const RenderTexture *cos_view);

void viewer_run(const ViewerOptions *options, const Scene *scene, const bool *should_run) {
    ViewerContext context = {
        .options = options,
        .scene_radius = get_scene_radius(scene),
        .display_hud = true,
        .display_cos = true,
    };

    // Create a resizable window
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(options->initial_window_width, options->initial_window_height, options->window_title);

    // Create all models of the scene
    Mesh mesh = {0};
    Mesh inverted_mesh = {0};
    Mesh wireframe_mesh = {0};
    set_empty_mesh_with_scene(scene, &mesh);
    set_empty_mesh_with_scene(scene, &inverted_mesh);
    invert_normals_mesh(&inverted_mesh);
    set_empty_mesh_with_scene(scene, &wireframe_mesh);
    set_mesh_color(options->edge_color, &wireframe_mesh);
    UploadMesh(&mesh, false);
    UploadMesh(&inverted_mesh, false);
    UploadMesh(&wireframe_mesh, false);
    Model model = LoadModelFromMesh(mesh);
    Model inverted_model = LoadModelFromMesh(inverted_mesh);
    Model wireframe_model = LoadModelFromMesh(wireframe_mesh);

    // Create a render texture for the coordinate system view
    RenderTexture cos_view = LoadRenderTexture(COS_VIEW_WIDTH, COS_VIEW_HEIGHT);

    // Set up the camera for the scene
    Camera camera = {0};
    reset_camera(&context, &camera);

    SetTargetFPS(TARGET_FPS);

    // Check for ending signal from cancelation token and GUI events
    while (*should_run && !WindowShouldClose()) {
        // Update the state of the viewer
        update_context(scene, &camera, &context);
        update_camera(&context, &camera);

        render_cos_view(&context, &camera, &cos_view);

        BeginDrawing();

        ClearBackground(options->background);

        BeginMode3D(camera);
        DrawModel(model, (Vector3){0, 0, 0}, 1, WHITE);
        if (options->render_facets_both_sides) DrawModel(inverted_model, (Vector3){0, 0, 0}, 1, WHITE);
        if (options->edge_color.a) DrawModelWires(wireframe_model, (Vector3){0, 0, 0}, 1, WHITE);
        draw_surface_selection(&context);
        EndMode3D();

        draw_rendered_cos_view(&context, &cos_view);

        draw_control_info(&context);
        draw_fps(&context);

        EndDrawing();

        create_screenshot();
    }

    // De-initialize resources
    UnloadRenderTexture(cos_view);
    UnloadModel(wireframe_model);
    UnloadModel(inverted_model);
    UnloadModel(model);
    CloseWindow();
}

// ****************************************************************************
// Scene
// ****************************************************************************

float get_scene_radius(const Scene *scene) {
    // Compare the squares for the max length cos length needs sqrt to compute
    // only compute the sqrt of the maximum
    // Add one to the found radius to ensure a little distance is always kept
    // Scene radius is only used to prevent clipping through objects for zooming the fov can be modified
    float max_length_sqr = 0.0f;

    for (size_t i_obj = 0; i_obj < scene->objects.length; ++i_obj) {
        Object obj = scene->objects.items[i_obj];

        for (size_t i_ver = 0; i_ver < obj.vertices.length; i_ver += 3) {
            Vector3 v = {obj.vertices.items[i_ver], obj.vertices.items[i_ver + 1], obj.vertices.items[i_ver + 2]};
            float length_sqr = Vector3LengthSqr(v);

            if (length_sqr > max_length_sqr) {
                max_length_sqr = length_sqr;
            }
        }
    }

    return 1.0f + sqrtf(max_length_sqr);
}

void set_empty_mesh_with_scene(const Scene *scene, Mesh *mesh) {
    size_t vertex_count = 0;
    for (size_t i = 0; i < scene->objects.length; ++i) {
        size_t obj_vertex_count = scene->objects.items[i].vertices.length / 3;

        assert(obj_vertex_count * 4 == scene->objects.items[i].colors.length &&
               "Dimension mismatch between colors and vertices");

        vertex_count += obj_vertex_count;
    }

    mesh->triangleCount = vertex_count / 3;
    mesh->vertexCount = vertex_count;

    mesh->vertices = (float *)MemAlloc(vertex_count * 3 * sizeof(float));
    mesh->colors = (unsigned char *)MemAlloc(vertex_count * 4 * sizeof(unsigned));

    size_t vertex_offset = 0;
    for (size_t i = 0; i < scene->objects.length; ++i) {
        size_t obj_vertex_count = scene->objects.items[i].vertices.length / 3;

        memcpy(&mesh->vertices[3 * vertex_offset], scene->objects.items[i].vertices.items,
               obj_vertex_count * 3 * sizeof(float));
        memcpy(&mesh->colors[4 * vertex_offset], scene->objects.items[i].colors.items,
               obj_vertex_count * 4 * sizeof(unsigned char));

        vertex_offset += obj_vertex_count;
    }
}

void invert_normals_mesh(Mesh *mesh) {
    for (size_t i = 0; i < mesh->vertexCount * 3; i += 9) {
        for (size_t i_comp = 0; i_comp < 3; ++i_comp) {
            float temp = mesh->vertices[i + 3 + i_comp];
            mesh->vertices[i + 3 + i_comp] = mesh->vertices[i + 6 + i_comp];
            mesh->vertices[i + 6 + i_comp] = temp;
        }
    }
}

void set_mesh_color(Color color, Mesh *mesh) {
    for (size_t i = 0; i < mesh->vertexCount * 4; i += 4) {
        mesh->colors[i] = color.r;
        mesh->colors[i + 1] = color.g;
        mesh->colors[i + 2] = color.b;
        mesh->colors[i + 3] = color.a;
    }
}

void draw_surface_selection(const ViewerContext *context) {
    if (!context->surface_selection.active) return;

    // Surface
    DrawTriangle3D(context->surface_selection.vertices[0], context->surface_selection.vertices[1],
                   context->surface_selection.vertices[2], RED);
    DrawTriangle3D(context->surface_selection.vertices[0], context->surface_selection.vertices[2],
                   context->surface_selection.vertices[1], RED);

    Vector3 rel = Vector3Scale(context->surface_selection.normal, context->scene_radius);
    Vector3 end = Vector3Add(context->surface_selection.point, rel);
    DrawLine3D(context->surface_selection.point, end, RED);
}

void create_screenshot() {
    // <CTRL> + "P" to amek a screenshot
    bool is_any_ctrl_down = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    if (is_any_ctrl_down && IsKeyPressed(KEY_P)) {
        printf("[SCREENSHOT] Enter the filename for the screenshot (leave blank to cancel): ");

        char filename[2048];
        if (!fgets(filename, 2048, stdin)) {
            fprintf(stderr, "[ERR] Could not read input. Screenshot creation is canceled.\n");
            return;
        }

        // Place a null termination on first new line so filename will contain a string to the inputed line
        int i = 0;
        while (filename[i] != '\r' && filename[i] != '\n' && filename[i] != '\0') ++i;
        filename[i] = '\0';

        // Cancel on empty input
        if (filename[0] == '\0') {
            printf("[SCREENSHOT] Screenshot creation is canceled.\n");
            return;
        }

        // Save screenshot to file
        TakeScreenshot(filename);
    }
}

// ****************************************************************************
// Viewer context
// ****************************************************************************

void update_context(const Scene *scene, const Camera *camera, ViewerContext *context) {
    // Toggle hub by pressing "H"
    if (IsKeyPressed(KEY_H)) {
        context->display_hud = !context->display_hud;
    }

    // Toggle coordinate system visualization by pressing "C"
    if (IsKeyPressed(KEY_C)) {
        context->display_cos = !context->display_cos;
    }

    // Select normal by pressing "S"
    if (IsKeyPressed(KEY_S)) {
        set_surface_selection(scene, camera, &context->surface_selection);
    }
}

void set_surface_selection(const Scene *scene, const Camera *camera, SurfaceSelection *surface_selection) {
    *surface_selection = (SurfaceSelection){0};  // Reset selection

    Vector2 pos = GetMousePosition();
    Ray ray = GetMouseRay(pos, *camera);

    RayCollision nearest_collision = {0};
    for (size_t i_obj = 0; i_obj < scene->objects.length; ++i_obj) {
        Object obj = scene->objects.items[i_obj];

        for (size_t i_ver = 0; i_ver < obj.vertices.length; i_ver += 9) {
            Vector3 v1 = {obj.vertices.items[i_ver], obj.vertices.items[i_ver + 1], obj.vertices.items[i_ver + 2]};
            Vector3 v2 = {obj.vertices.items[i_ver + 3], obj.vertices.items[i_ver + 4], obj.vertices.items[i_ver + 5]};
            Vector3 v3 = {obj.vertices.items[i_ver + 6], obj.vertices.items[i_ver + 7], obj.vertices.items[i_ver + 8]};

            RayCollision collision = GetRayCollisionTriangle(ray, v1, v2, v3);

            if (!collision.hit) continue;

            if (!nearest_collision.hit || collision.distance < nearest_collision.distance) {
                nearest_collision = collision;
                surface_selection->active = true;
                surface_selection->point = collision.point;
                surface_selection->normal = collision.normal;
                surface_selection->vertices[0] = v1;
                surface_selection->vertices[1] = v2;
                surface_selection->vertices[2] = v3;
            }
        }
    }
}

// ****************************************************************************
// Camera control
// ****************************************************************************

void reset_camera(const ViewerContext *context, Camera *camera) {
    camera->position = (Vector3){-1.5 * context->scene_radius, 0, 0};
    camera->target = (Vector3){0, 0, 0};
    camera->up = (Vector3){0, 0, 1};
    camera->fovy = 45.0f;
    camera->projection = CAMERA_ORTHOGRAPHIC;
}

void update_camera(const ViewerContext *context, Camera *camera) {
    // Rotate via left mouse button down + drag
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        Vector2 mouse_delta = GetMouseDelta();

        // Transform camera orientation representation from (pos, tar, up) -> (view, up, right)
        Vector3 view = Vector3Subtract(camera->target, camera->position);
        Vector3 up = camera->up;
        Vector3 right = Vector3CrossProduct(view, up);

        // Handle up - down (rotation along right)
        float up_down_angle = -ROTATION_SENSITIVITY * mouse_delta.y;
        view = Vector3RotateByAxisAngle(view, right, up_down_angle);
        up = Vector3RotateByAxisAngle(up, right, up_down_angle);

        // Handle left - right (rotation along up)
        float left_right_angle = -ROTATION_SENSITIVITY * mouse_delta.x;
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
        float pan_factor = PAN_SENEITIVITY * camera->fovy / GetScreenWidth();
        float up_distance = pan_factor * mouse_delta.y;
        Vector3 up_pan = Vector3Scale(up_unit, up_distance);

        float right_distance = -pan_factor * mouse_delta.x;
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
    if (IsKeyPressed(KEY_R)) {
        reset_camera(context, camera);
    }

    // Rotate normal to selected surface  via "N"
    if (context->surface_selection.active && IsKeyPressed(KEY_N)) {
        Vector3 up = camera->up;
        Vector3 normal = context->surface_selection.normal;

        // Ensure up and normal are not parallel and adjust up if so
        Vector3 right = Vector3CrossProduct(up, normal);
        if (Vector3LengthSqr(right) < EPSILON) {
            // First fallback is unit z
            up = (Vector3){0, 0, 1};
            right = Vector3CrossProduct(up, normal);
            if (Vector3LengthSqr(right) < EPSILON) {
                // If up and normal is +/- unitz use unit x
                up = (Vector3){1, 0, 0};
                right = Vector3CrossProduct(up, normal);
            }
        }

        // ensure up is normalized and orthogonal to normal
        right = Vector3Normalize(right);  // Normalize the vector to increase numeric accuracy
        up = Vector3CrossProduct(normal, right);
        up = Vector3Normalize(up);

        Vector3 relative = Vector3Scale(normal, 1.5 * context->scene_radius);

        camera->up = up;
        camera->target = context->surface_selection.point;
        camera->position = Vector3Add(camera->target, relative);
    }
}

// ****************************************************************************
// HUD
// ****************************************************************************

#define draw_control_entry(binding, description)                              \
    do {                                                                      \
        DrawRectangle(8, 8 + 16 * entry, 520, 16, colors[(entry + 1) % 2]);   \
        DrawText((binding), 10, 10 + 16 * entry, 12, colors[entry % 2]);      \
        DrawText((description), 250, 10 + 16 * entry, 12, colors[entry % 2]); \
        ++entry;                                                              \
    } while (0)

void draw_control_info(const ViewerContext *context) {
    Color colors[2] = {DARKGRAY, LIGHTGRAY};
    size_t entry = 0;

    if (context->display_hud) {
        draw_control_entry("H", "Toggle HUD");
        draw_control_entry("C", "Toggle coordinate system visualization");
        draw_control_entry("S", "Select surface");
        draw_control_entry("N", "Move camera normal to the select surface");
        draw_control_entry("R", "Reset camera");
        draw_control_entry("CTRL + P", "Create screenshot (Enter filename to stdin)");
        draw_control_entry("Left Mouse Button + Mouse Drag", "Rotate");
        draw_control_entry("Right Mouse Button + Mouse Drag", "Pan");
        draw_control_entry("Mouse Wheel", "Zoom");
    } else if (!context->options->true_no_hud) {
        // Only show the toggle HUD control when the hud is hidden
        // Only when the options explicitly ask for no hud, hide this option
        draw_control_entry("H", "Toggle HUD");
    }
}

void draw_fps(const ViewerContext *context) {
    if (!context->display_hud) return;

    int scree_width = GetScreenWidth();
    DrawFPS(scree_width - 100, 10);
}

// ****************************************************************************
// Visualization of the coordinate system
// ****************************************************************************

void draw_arrow(Vector3 start, Vector3 dir_normalized, float line_length, float line_radius, float tip_length,
                float tip_radius, int sides, Color color) {
    // Compte the points for the cylinders
    Vector3 mid = Vector3Add(start, Vector3Scale(dir_normalized, line_length));
    Vector3 tip = Vector3Add(mid, Vector3Scale(dir_normalized, tip_length));

    DrawCylinderEx(start, mid, line_radius, line_radius, sides, color);  // Line
    DrawCylinderEx(mid, tip, tip_radius, 0.0, sides, color);             // Tip
}

void render_cos_view(const ViewerContext *context, Camera *camera, RenderTexture *cos_view) {
    if (!context->display_cos) return;

    // Options of the arrow geometries
    int sides = 10;
    float line_length = 1.0;
    float tip_length = 0.5;
    float origin_radius = 0.2;
    float line_radius = 0.05;
    float tip_radius = 0.2;

    // Use camera with same orientation but fixed target and fov
    Camera view_camera = {0};
    view_camera.position = Vector3Scale(Vector3Normalize(Vector3Subtract(camera->position, camera->target)), 3.0);
    view_camera.up = camera->up;
    view_camera.fovy = 4.0f;
    view_camera.projection = CAMERA_ORTHOGRAPHIC;

    BeginTextureMode(*cos_view);

    // Set transparent background
    ClearBackground((Color){0, 0, 0, 0});

    BeginMode3D(view_camera);

    draw_arrow((Vector3){0, 0, 0}, (Vector3){1, 0, 0}, line_length, line_radius, tip_length, tip_radius, sides, RED);
    draw_arrow((Vector3){0, 0, 0}, (Vector3){0, 1, 0}, line_length, line_radius, tip_length, tip_radius, sides, GREEN);
    draw_arrow((Vector3){0, 0, 0}, (Vector3){0, 0, 1}, line_length, line_radius, tip_length, tip_radius, sides, BLUE);
    DrawSphere((Vector3){0, 0, 0}, origin_radius, BLACK);  // Oriding

    EndMode3D();

    EndTextureMode();
}

void draw_rendered_cos_view(const ViewerContext *context, const RenderTexture *cos_view) {
    if (!context->display_cos) return;

    // Draw the texture in the bottom left corner
    Vector2 position = {0, GetScreenHeight() - cos_view->texture.height};

    // Build the flipped rectangle of the cos view for drawing
    Rectangle source = {0, 0, cos_view->texture.width, -cos_view->texture.height};

    DrawTextureRec(cos_view->texture, source, position, WHITE);
}
