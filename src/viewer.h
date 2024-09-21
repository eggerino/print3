#ifndef PRINT3_VIEWER_H_
#define PRINT3_VIEWER_H_

#include <stdbool.h>

#include "scene.h"

typedef struct ViewerOptions {
    const char *window_title;
    int initial_window_width;
    int initial_window_height;
    Color background;
    bool render_facets_both_sides;
} ViewerOptions;

void viewer_run(const ViewerOptions *opt, const Scene *scene, const bool *should_run);

#endif
