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

        if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--both-sides") == 0) {
            args->viewer.render_facets_both_sides = true;
            continue;
        }

        if (strcmp(argv[i], "-bg") == 0 || strcmp(argv[i], "--background") == 0) {
            if (i + 3 >= argc) {
                fprintf(stderr,
                        "[ERR] Three arguments must be provided after the background option but only %d are given.\n",
                        argc - i - 1);
                for (int i_arg = i + 1; i_arg < argc; ++i_arg) {
                    fprintf(stderr, "[%d]: %s\n", i_arg - i, argv[i_arg]);
                }

                usage(stderr, prog);
                exit(1);
            }

            // Check for invalid inputs
            char *names[] = {"red", "green", "blue"};
            for (size_t i_comp = 0; i_comp < 3; ++i_comp) {
                if (!isdigit(argv[i + 1 + i_comp][0])) {
                    fprintf(stderr, "[ERR] %d. component (%s) of the background must be a number. %s was given.\n",
                            i_comp + 1, names[i_comp], argv[i + 1 + i_comp]);
                    usage(stderr, prog);
                    exit(1);
                }
            }

            char red = atoi(argv[i + 1]);
            char green = atoi(argv[i + 2]);
            char blue = atoi(argv[i + 3]);
            i += 3;

            args->viewer.background = (Color){red, green, blue, 255};
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

void usage(FILE *stream, const char *prog_name) {
    fprintf(stream,
            "[USAGE] %s [OPTION ...] [INPUT [INPUT ...]]\n"
            "\n"
            "- INPUT: \"STDIN\" | FILE\n"
            "    Specify a file with a 3d model to visualize or provide the data via stdin.\n"
            "    The same file can be specified multiple times and will be duplicately rendered.\n"
            "    If \"STDIN\" is specified n times, n distinct objects need to be specified in stdin before rendering can "
            "take place.\n"
            "    The following file formats are supported:\n"
            "    - none\n"
            "    The data format for stdin is the following:\n"
            "    - Provide the color of the surface in the first line\n"
            "        [red: UINT8] [green: UINT8] [blue: UINT8] [alpha: UINT8]\n"
            "    - Provide for each surface three vertices with there x-, y-, z-coordiantes per line\n"
            "        [v1x: REAL32] [v1y: REAL32] [v1z: REAL32] [v2x: REAL32] [v2y: REAL32] [v2z: REAL32] [v3x: REAL32] "
            "[v3y: REAL32] [v3z: REAL32]\n"
            "    - Terminate the specification with \"end\"\n"
            "\n"
            "- OPTION:\n"
            "    -h  | --help           (FLAG) Default: false\n"
            "                           Print help message.\n"
            "\n"
            "    -nh | --no-hud         (FLAG) Default: false\n"
            "                           When toggling the HUD the entire hud will be hidden. Even the control hint for "
            "toggling the hud.\n"
            "\n"
            "    -s  | --both-sides     (FLAG) Default: false\n"
            "                           Each surface will be rendered twice but with opposite normal vectors.\n"
            "                           So each surface is guaranteed to be visible by the camera.\n"
            "                           With a proper 3D model this option should not be required.\n"
            "\n"
            "    -bg | --background     Default: 255 255 255 (white)\n"
            "                           Background color of the scene in the format [red: UINT8] [green: UINT8] [blue: "
            "UINT8].\n");
}
