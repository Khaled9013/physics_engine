#include "cli_options.h"

#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

static BallisticsStatus parse_positive_double(const char *text, double *out_value)
{
    char *end = NULL;
    double value;

    if (text == NULL || out_value == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    errno = 0;
    value = strtod(text, &end);
    if (errno != 0 || end == text || *end != '\0' || !isfinite(value) || value <= 0.0)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    *out_value = value;
    return BALLISTICS_STATUS_OK;
}

static BallisticsStatus parse_debug_level(const char *text, BallisticsDebugLevel *out_level)
{
    static const char *const names[] = {"error", "warning", "info", "debug", "trace"};
    size_t index;

    if (text == NULL || out_level == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    for (index = 0U; index < sizeof(names) / sizeof(names[0]); ++index)
    {
        if (strcmp(text, names[index]) == 0)
        {
            *out_level = (BallisticsDebugLevel)index;
            return BALLISTICS_STATUS_OK;
        }
    }
    return BALLISTICS_STATUS_INVALID_ARGUMENT;
}

BallisticsStatus ballistics_cli_options_parse(int argc,
                                              char **argv,
                                              BallisticsCliOptions *out_options)
{
    int index;

    if (argc < 1 || argv == NULL || out_options == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    *out_options = (BallisticsCliOptions){
        "rk4.v1", 0.001, 10.0, "trajectory.csv", BALLISTICS_DEBUG_LEVEL_WARNING, false};
    for (index = 1; index < argc; ++index)
    {
        const char *option = argv[index];
        if (strcmp(option, "--help") == 0 || strcmp(option, "-h") == 0)
        {
            out_options->show_help = true;
            continue;
        }
        if (index + 1 >= argc)
        {
            return BALLISTICS_STATUS_INVALID_ARGUMENT;
        }
        ++index;
        if (strcmp(option, "--integrator") == 0)
        {
            out_options->integrator_id = argv[index];
        }
        else if (strcmp(option, "--time-step") == 0)
        {
            if (parse_positive_double(argv[index], &out_options->time_step_s) !=
                BALLISTICS_STATUS_OK)
            {
                return BALLISTICS_STATUS_INVALID_ARGUMENT;
            }
        }
        else if (strcmp(option, "--max-time") == 0)
        {
            if (parse_positive_double(argv[index], &out_options->maximum_time_s) !=
                BALLISTICS_STATUS_OK)
            {
                return BALLISTICS_STATUS_INVALID_ARGUMENT;
            }
        }
        else if (strcmp(option, "--output") == 0)
        {
            out_options->output_path = argv[index];
        }
        else if (strcmp(option, "--debug-level") == 0)
        {
            if (parse_debug_level(argv[index], &out_options->debug_level) !=
                BALLISTICS_STATUS_OK)
            {
                return BALLISTICS_STATUS_INVALID_ARGUMENT;
            }
        }
        else
        {
            return BALLISTICS_STATUS_INVALID_ARGUMENT;
        }
    }
    return BALLISTICS_STATUS_OK;
}

void ballistics_cli_options_print_usage(FILE *stream, const char *program_name)
{
    if (stream == NULL || program_name == NULL)
    {
        return;
    }
    fprintf(stream,
            "Usage: %s [--integrator ID] [--time-step S] [--max-time S] "
            "[--output PATH] [--debug-level LEVEL]\n",
            program_name);
}
