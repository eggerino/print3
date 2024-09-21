#include "args.h"
#include "scene.h"
#include "viewer.h"

int main(int argc, const char **argv) {
    Args args = {0};
    args_parse(argc, argv, &args);

    Scene scene = {0};
    scene_add_demo_object(&scene);

    bool viewer_should_run = true;
    viewer_run(&args.viewer, &scene, &viewer_should_run);

    scene_free_members(&scene);
    args_free_member(&args);

    return 0;
}
