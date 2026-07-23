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
    double maximum_distance_m;
    const char *output_path;
    BallisticsDebugLevel debug_level;
    double projectile_mass_kg;
    double projectile_diameter_m;
    double reference_area_m2;
    double launch_speed_mps;
    double elevation_deg;
    double azimuth_deg;
    double initial_height_m;
    double drag_coefficient;
    double air_density_kgpm3;
    double wind_x_mps;
    double wind_y_mps;
    double wind_z_mps;
    double gravity_mps2;
    bool show_help;
} BallisticsCliOptions;

BallisticsStatus ballistics_cli_options_parse(int argc,
                                              char **argv,
                                              BallisticsCliOptions *out_options);
void ballistics_cli_options_print_usage(FILE *stream, const char *program_name);

#endif
