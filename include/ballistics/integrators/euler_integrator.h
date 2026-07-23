#ifndef BALLISTICS_INTEGRATORS_EULER_INTEGRATOR_H
#define BALLISTICS_INTEGRATORS_EULER_INTEGRATOR_H

#include "ballistics/interfaces/integrator_interface.h"
#include <stddef.h>

#define BALLISTICS_EULER_INTEGRATOR_ID "euler.v1"
#define BALLISTICS_EULER_MAXIMUM_STATE_COUNT 64U

typedef struct
{
    size_t maximum_state_count;
} BallisticsEulerIntegratorConfig;

BALLISTICS_API BallisticsStatus ballistics_euler_integrator_create(
    const BallisticsEulerIntegratorConfig *config,
    BallisticsIntegrator **out_integrator);
BALLISTICS_API BallisticsStatus ballistics_euler_integrator_factory(
    const void *config, size_t config_size, BallisticsIntegrator **out_integrator);

#endif
