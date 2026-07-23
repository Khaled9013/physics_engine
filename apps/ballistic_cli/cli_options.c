#include "cli_options.h"

#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

static BallisticsStatus parse_finite_double(const char *text, double *out_value)
{
    char *end = NULL;
    double value;

    if (text == NULL || out_value == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    errno = 0;
    value = strtod(text, &end);
    if (errno != 0 || end == text || *end != '\0' || !isfinite(value))
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    *out_value = value;
    return BALLISTICS_STATUS_OK;
}

static BallisticsStatus parse_positive_double(const char *text, double *out_value)
{
    BallisticsStatus status = parse_finite_double(text, out_value);

    if (status != BALLISTICS_STATUS_OK || *out_value <= 0.0)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    return BALLISTICS_STATUS_OK;
}

static BallisticsStatus parse_nonnegative_double(const char *text, double *out_value)
{
    BallisticsStatus status = parse_finite_double(text, out_value);

    if (status != BALLISTICS_STATUS_OK || *out_value < 0.0)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    return BALLISTICS_STATUS_OK;
}

static BallisticsStatus parse_angle_double(const char *text,
                                            double minimum_deg,
                                            double maximum_deg,
                                            double *out_value)
{
    BallisticsStatus status = parse_finite_double(text, out_value);

    if (status != BALLISTICS_STATUS_OK || *out_value < minimum_deg ||
        *out_value > maximum_deg)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
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

static BallisticsStatus parse_numeric_option(const char *option,
                                              const char *value,
                                              BallisticsCliOptions *options)
{
    if (strcmp(option, "--time-step") == 0)
    {
        return parse_positive_double(value, &options->time_step_s);
    }
    if (strcmp(option, "--max-time") == 0)
    {
        return parse_positive_double(value, &options->maximum_time_s);
    }
    if (strcmp(option, "--max-distance") == 0)
    {
        return parse_positive_double(value, &options->maximum_distance_m);
    }
    if (strcmp(option, "--mass") == 0)
    {
        return parse_positive_double(value, &options->projectile_mass_kg);
    }
    if (strcmp(option, "--diameter") == 0)
    {
        return parse_nonnegative_double(value, &options->projectile_diameter_m);
    }
    if (strcmp(option, "--reference-area") == 0)
    {
        return parse_positive_double(value, &options->reference_area_m2);
    }
    if (strcmp(option, "--launch-speed") == 0)
    {
        return parse_nonnegative_double(value, &options->launch_speed_mps);
    }
    if (strcmp(option, "--elevation-deg") == 0)
    {
        return parse_angle_double(value, -90.0, 90.0, &options->elevation_deg);
    }
    if (strcmp(option, "--azimuth-deg") == 0)
    {
        return parse_angle_double(value, -180.0, 180.0, &options->azimuth_deg);
    }
    if (strcmp(option, "--initial-height") == 0)
    {
        return parse_nonnegative_double(value, &options->initial_height_m);
    }
    if (strcmp(option, "--drag-coefficient") == 0)
    {
        return parse_nonnegative_double(value, &options->drag_coefficient);
    }
    if (strcmp(option, "--air-density") == 0)
    {
        return parse_nonnegative_double(value, &options->air_density_kgpm3);
    }
    if (strcmp(option, "--wind-x") == 0)
    {
        return parse_finite_double(value, &options->wind_x_mps);
    }
    if (strcmp(option, "--wind-y") == 0)
    {
        return parse_finite_double(value, &options->wind_y_mps);
    }
    if (strcmp(option, "--wind-z") == 0)
    {
        return parse_finite_double(value, &options->wind_z_mps);
    }
    if (strcmp(option, "--gravity") == 0)
    {
        return parse_nonnegative_double(value, &options->gravity_mps2);
    }
    return BALLISTICS_STATUS_NOT_FOUND;
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
        .integrator_id = "rk4.v1",
        .time_step_s = 0.001,
        .maximum_time_s = 10.0,
        .maximum_distance_m = 5000.0,
        .output_path = "trajectory.csv",
        .debug_level = BALLISTICS_DEBUG_LEVEL_WARNING,
        .projectile_mass_kg = 0.018,
        .projectile_diameter_m = 0.009,
        .reference_area_m2 = 6.3617e-5,
        .launch_speed_mps = 310.0,
        .elevation_deg = 4.573921259900861,
        .azimuth_deg = 0.0,
        .initial_height_m = 1.5,
        .drag_coefficient = 0.29,
        .air_density_kgpm3 = 1.225,
        .wind_x_mps = 0.0,
        .wind_y_mps = 2.0,
        .wind_z_mps = 0.0,
        .gravity_mps2 = 9.80665,
        .show_help = false};

    for (index = 1; index < argc; ++index)
    {
        const char *option = argv[index];
        BallisticsStatus status;

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
            if (argv[index][0] == '\0')
            {
                return BALLISTICS_STATUS_INVALID_ARGUMENT;
            }
            out_options->integrator_id = argv[index];
            continue;
        }
        if (strcmp(option, "--output") == 0)
        {
            if (argv[index][0] == '\0')
            {
                return BALLISTICS_STATUS_INVALID_ARGUMENT;
            }
            out_options->output_path = argv[index];
            continue;
        }
        if (strcmp(option, "--debug-level") == 0)
        {
            status = parse_debug_level(argv[index], &out_options->debug_level);
        }
        else
        {
            status = parse_numeric_option(option, argv[index], out_options);
        }
        if (status != BALLISTICS_STATUS_OK)
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
            "Usage: %s [options]\n"
            "  --integrator ID          rk4.v1 or euler.v1\n"
            "  --time-step S            fixed integration step\n"
            "  --max-time S             maximum simulated time\n"
            "  --max-distance M         horizontal distance limit\n"
            "  --output PATH            trajectory CSV path\n"
            "  --mass KG                 synthetic projectile mass\n"
            "  --diameter M              synthetic projectile diameter\n"
            "  --reference-area M2       aerodynamic reference area\n"
            "  --launch-speed MPS        initial speed\n"
            "  --elevation-deg DEG       launch elevation [-90, 90]\n"
            "  --azimuth-deg DEG         launch azimuth [-180, 180]\n"
            "  --initial-height M        initial height above ground\n"
            "  --drag-coefficient CD     constant drag coefficient\n"
            "  --air-density KGPM3       constant atmospheric density\n"
            "  --wind-x MPS              downrange wind component\n"
            "  --wind-y MPS              rightward wind component\n"
            "  --wind-z MPS              upward wind component\n"
            "  --gravity MPS2            downward gravity magnitude\n"
            "  --debug-level LEVEL       error|warning|info|debug|trace\n",
            program_name);
}
