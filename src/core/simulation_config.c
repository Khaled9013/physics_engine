#include "ballistics/simulation_config.h"

#include <math.h>
#include <stddef.h>

BallisticsStatus ballistics_simulation_config_validate(
    const BallisticsSimulationConfig *config,
    const BallisticsIntegratorRegistry *integrator_registry)
{
    BallisticsIntegratorFactory factory = NULL;

    if (config == NULL || !isfinite(config->time_step_seconds) ||
        config->time_step_seconds <= 0.0 || !isfinite(config->maximum_time_seconds) ||
        config->maximum_time_seconds <= 0.0 || !isfinite(config->maximum_distance_metres) ||
        config->maximum_distance_metres <= 0.0 || !isfinite(config->ground_height_metres) ||
        config->integrator_id == NULL || config->integrator_id[0] == '\0')
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    if (integrator_registry != NULL)
    {
        return ballistics_integrator_registry_find(
            integrator_registry, config->integrator_id, &factory);
    }
    return BALLISTICS_STATUS_OK;
}
