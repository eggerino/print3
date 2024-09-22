#include "args.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "raylib.h"

static void handle_help(int argc, const char **argv);
static void set_defaults(Args *args);
static int parse_options(int argc, const char **argv, int start, Args *args);
static void parse_inputs(int argc, const char **argv, int start, Args *args);
static void define_window_title(Args *args);
static void warn_on_unusual_args(const Args *args);
static Color parse_color(int argc, const char **argv, int offset, int channels);
static void usage(FILE *stream, const char *prog_name);

void args_parse(int argc, const char **argv, Args *args) {
    handle_help(argc, argv);

    set_defaults(args);

    int iarg = parse_options(argc, argv, 1, args);
    parse_inputs(argc, argv, iarg, args);

    define_window_title(args);

    warn_on_unusual_args(args);
}

void args_free_member(Args *args) {
    free(args->files.items);
    free(args->viewer.window_title);
}

void args_print(const Args *args) {
    printf("Input arguments:\n");
    printf("- stdin object count: %d\n", args->stdin_object_count);
    printf("- file count: %d\n", args->files.length);
    for (int i = 0; i < args->files.length; ++i) {
        printf("  - file[%d] = %s\n", i, args->files.items[i]);
    }

    printf("\nViewer arguments:\n");
    printf("- window title: %s\n", args->viewer.window_title);
    printf("- initial window size: %d x %d\n", args->viewer.initial_window_width, args->viewer.initial_window_height);
    printf("- no hud: %d\n", args->viewer.true_no_hud);
    printf("- background color: r=%d g=%d b=%d a=%d\n", args->viewer.background.r, args->viewer.background.g,
           args->viewer.background.b, args->viewer.background.a);
    printf("- both sides: %d\n", args->viewer.render_facets_both_sides);
}

void handle_help(int argc, const char **argv) {
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            usage(stdout, argv[0]);
            exit(0);
        }
    }
}

void set_defaults(Args *args) {
    args->stdin_object_count = 0;

    args->fallback_color = BLUE;

    args->viewer.initial_window_width = 1600;
    args->viewer.initial_window_height = 900;

    args->viewer.true_no_hud = false;
    args->viewer.background = WHITE;
    args->viewer.render_facets_both_sides = false;
}

int parse_options(int argc, const char **argv, int start, Args *args) {
    const char *prog = argv[0];

    int i = start;
    for (; i < argc; ++i) {
        if (strcmp(argv[i], "-nh") == 0 || strcmp(argv[i], "--no-hud") == 0) {
            args->viewer.true_no_hud = true;
            continue;
        }

        if (strcmp(argv[i], "-b") == 0 || strcmp(argv[i], "--both-sides") == 0) {
            args->viewer.render_facets_both_sides = true;
            continue;
        }

        if (strcmp(argv[i], "-bg") == 0 || strcmp(argv[i], "--background") == 0) {
            args->viewer.background = parse_color(argc, argv, i + 1, 3);
            i += 3;
            continue;
        }

        if (strcmp(argv[i], "-fc") == 0 || strcmp(argv[i], "--fallback-color") == 0) {
            args->fallback_color = parse_color(argc, argv, i + 1, 4);
            i += 4;
            continue;
        }

        return i;
    }

    return i;
}

void parse_inputs(int argc, const char **argv, int start, Args *args) {
    for (int i = start; i < argc; ++i) {
        if (strcmp(argv[i], "STDIN") == 0) {
            args->stdin_object_count++;
            continue;
        }

        da_add(args->files, argv[i]);
    }
}

void define_window_title(Args *args) {
    // Determine the size of the window title buffer
    size_t n = 7;  // print3\n
    for (size_t i = 0; i < args->files.length; ++i) {
        n += 2;                             // Delimiter ": " or ", "
        n += strlen(args->files.items[i]);  // file path
    }

    char *title = malloc(n * sizeof(char));
    memcpy(title, "print3", 6);

    size_t i_title = 6;
    for (size_t i = 0; i < args->files.length; ++i) {
        // write delimiter
        title[i_title++] = i == 0 ? ':' : ',';
        title[i_title++] = ' ';

        // file path
        size_t n_file = strlen(args->files.items[i]);
        memcpy(&title[i_title], args->files.items[i], n_file);
        i_title += n_file;
    }

    title[n - 1] = '\0';

    args->viewer.window_title = title;
}

void warn_on_unusual_args(const Args *args) {
    if ((args->stdin_object_count + args->files.length) == 0) {
        fprintf(stderr, "[WARN] No input was specified. Only a empty scene will be visualized.\n");
        fprintf(stderr, "       Consider specify a input file or \"STDIN\" to provide an object via stdin.\n");
    }
}

Color parse_color(int argc, const char **argv, int offset, int channels) {
    assert(channels <= 4 && "Maximum of 4 channels is supported.");

    // Check for correct number of arguments
    if (offset + channels > argc) {
        fprintf(stderr, "[ERR] %d arguments must be provided for a color but only %d are given.\n", channels, argc - offset);

        for (size_t i = offset; i < argc; ++i) {
            fprintf(stderr, "[%d]: %s\n", i - offset + 1, argv[i]);
        }

        usage(stderr, argv[0]);
        exit(1);
    }

    // Check for invalid inputs
    char comp[4] = {0, 0, 0, 255};
    char *names[] = {"red", "green", "blue", "alpha"};

    char *peak;
    for (size_t i = 0; i < channels; ++i) {
        comp[i] = strtol(argv[i + offset], &peak, 10);
        if (peak == argv[i + offset]) {
            fprintf(stderr, "[ERR] %d. component (%s) of the color must be a number. %s was given.\n", i + 1, names[i],
                    argv[i + offset]);
            usage(stderr, argv[0]);
            exit(1);
        }
    }

    return (Color){comp[0], comp[1], comp[2], comp[3]};
}

void usage(FILE *stream, const char *prog_name) {
    fprintf(stream,
            "[USAGE] %s [OPTION ...] [INPUT ...]]\n"
            "\n"
            "\n"
            "- INPUT: \"STDIN\" | FILE\n"
            "    Specify a file with a 3d model to visualize or provide the data via stdin.\n"
            "    The same file can be specified multiple times and will be duplicately rendered.\n"
            "    If \"STDIN\" is specified n times, n distinct objects need to be provided via stdin\n"
            "    before rendering can take place.\n"
            "\n"
            "    The following file formats are supported:\n"
            "    - stl (binary and ascii)\n"
            "\n"
            "    The data format for stdin is the following:\n"
            "    - Provide the color of the surface in the first line\n"
            "        {red: UINT8} {green: UINT8} {blue: UINT8} {alpha: UINT8}\n"
            "    - For each sufrace provide three vertices with their x-, y-, z-coordiantes per line each as REAL32\n"
            "        v1x v1y v1z v2x v2y v2z v3x v3y v3z\n"
            "    - Terminate the specification with \"end\" or EoF (only feasable for the last object)\n"
            "\n"
            "\n"
            "- OPTION:\n"
            "    -h  | --help           Default: false\n"
            "                           Format: Flag\n"
            "                           Print help message.\n"
            "\n"
            "    -nh | --no-hud         Default: false\n"
            "                           Format: Flag\n"
            "                           When toggling the HUD the entire hud will be hidden.\n"
            "                           Even the control hint for toggling the hud.\n"
            "\n"
            "    -b  | --both-sides     Default: false\n"
            "                           Format: Flag\n"
            "                           Each surface will be rendered twice but with opposite normal vectors.\n"
            "                           So each surface is guaranteed to be visible by the camera.\n"
            "                           With a proper 3D model this option should not be used.\n"
            "\n"
            "    -bg | --background     Default: 255 255 255 (white)\n"
            "                           Format: {red: UINT8} {green: UINT8} {blue: UINT8}\n"
            "                           Background color of the scene in the format.\n"
            "\n"
            "    -fc | --fallback-color Default: 0 121 241 255 (blue)\n"
            "                           Format: {red: UINT8} {green: UINT8} {blue: UINT8} {alpha: UINT8}\n"
            "                           Fallback color of objects. This is used, when there is no color information\n"
            "                           available for an object.\n"
            "\n",
            prog_name);
}
