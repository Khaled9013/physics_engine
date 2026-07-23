#ifndef BALLISTICS_SIMULATION_CONFIG_H
#define BALLISTICS_SIMULATION_CONFIG_H

#include "ballistics/export.h"
#include "ballistics/registry/integrator_registry.h"
#include "ballistics/status.h"

typedef struct
{
    double time_step_seconds;
    double maximum_time_seconds;
    /** Horizontal x-y distance from initial position; altitude is excluded. */
    double maximum_distance_metres;
    double ground_height_metres;
    const char *integrator_id;
} BallisticsSimulationConfig;

/** Validate values and, when non-null, resolve integrator_id in the borrowed registry. */
BALLISTICS_API BallisticsStatus ballistics_simulation_config_validate(
    const BallisticsSimulationConfig *config,
    const BallisticsIntegratorRegistry *integrator_registry);

#endif
