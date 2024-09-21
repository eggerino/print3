#include "scene.h"
#include "viewer.h"

int main(void) {
    Scene scene = {0};
    scene_add_demo_object(&scene);

    bool viewer_should_run = true;
    ViewerOptions opt = {"print3", 1600, 900, false, WHITE, false};
    viewer_run(&opt, &scene, &viewer_should_run);

    scene_free_members(&scene);

    return 0;
}
