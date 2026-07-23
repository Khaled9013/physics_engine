#ifndef BALLISTICS_INTEGRATORS_RK4_INTEGRATOR_H
#define BALLISTICS_INTEGRATORS_RK4_INTEGRATOR_H

#include "ballistics/interfaces/integrator_interface.h"
#include <stddef.h>

#define BALLISTICS_RK4_INTEGRATOR_ID "rk4.v1"
#define BALLISTICS_RK4_MAXIMUM_STATE_COUNT 64U

typedef struct
{
    size_t maximum_state_count;
} BallisticsRk4IntegratorConfig;

BALLISTICS_API BallisticsStatus ballistics_rk4_integrator_create(
    const BallisticsRk4IntegratorConfig *config,
    BallisticsIntegrator **out_integrator);
BALLISTICS_API BallisticsStatus ballistics_rk4_integrator_factory(
    const void *config, size_t config_size, BallisticsIntegrator **out_integrator);

#endif
