#ifndef BALLISTICS_SIMULATION_H
#define BALLISTICS_SIMULATION_H

#include "ballistics/dynamics.h"
#include "ballistics/export.h"
#include "ballistics/interfaces/integrator_interface.h"
#include "ballistics/interfaces/stop_condition_interface.h"
#include "ballistics/simulation_config.h"
#include "ballistics/simulation_result.h"

typedef struct BallisticsSimulation BallisticsSimulation;

/** Create a reusable simulation that borrows dynamics, integrator, condition array and conditions. */
BALLISTICS_API BallisticsStatus ballistics_simulation_create(
    const BallisticsSimulationConfig *config,
    const BallisticsProjectileState *initial_state,
    BallisticsDynamicsContext *dynamics,
    BallisticsIntegrator *integrator,
    BallisticsStopCondition *const *stop_conditions,
    size_t stop_condition_count,
    BallisticsSimulation **out_simulation);
BALLISTICS_API void ballistics_simulation_destroy(BallisticsSimulation *simulation);
BALLISTICS_API BallisticsStatus ballistics_simulation_run(
    BallisticsSimulation *simulation,
    BallisticsSimulationResult *result);

#endif
