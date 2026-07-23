#ifndef BALLISTICS_CLI_OPTIONS_H
#define BALLISTICS_CLI_OPTIONS_H

#include "ballistics/debug/ballistics_debug.h"
#include "ballistics/status.h"
#include <stdbool.h>
#include <stdio.h>

typedef struct
{
    const char *integrator_id;
    double time_step_s;
    double maximum_time_s;
    const char *output_path;
    BallisticsDebugLevel debug_level;
    bool show_help;
} BallisticsCliOptions;

BallisticsStatus ballistics_cli_options_parse(int argc,
                                              char **argv,
                                              BallisticsCliOptions *out_options);
void ballistics_cli_options_print_usage(FILE *stream, const char *program_name);

#endif
