#ifndef PRINT3_VIEWER_H_
#define PRINT3_VIEWER_H_

#include <stdbool.h>

#include "scene.h"

typedef struct ViewerOptions {
    char *window_title;
    int initial_window_width;
    int initial_window_height;
    bool true_no_hud;
    Color background;
    bool render_facets_both_sides;
    Color edge_color;
} ViewerOptions;

void viewer_run(const ViewerOptions *options, const Scene *scene, const bool *should_run);

#endif
