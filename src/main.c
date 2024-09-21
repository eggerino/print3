#include "args.h"
#include "deserialize/stdin.h"
#include "scene.h"
#include "viewer.h"

int main(int argc, const char **argv) {
    Args args = {0};
    args_parse(argc, argv, &args);

    Scene scene = {0};

    for (size_t i = 0; i < args.stdin_object_count; ++i) {
        stdin_add_to_scene(stdin, &scene);
    }

    bool viewer_should_run = true;
    viewer_run(&args.viewer, &scene, &viewer_should_run);

    scene_free_members(&scene);
    args_free_member(&args);

    return 0;
}
