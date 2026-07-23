#ifndef BALLISTICS_SIMULATION_RESULT_INTERNAL_H
#define BALLISTICS_SIMULATION_RESULT_INTERNAL_H

#include "ballistics/simulation_result.h"

BallisticsStatus ballistics_simulation_result_append(
    BallisticsSimulationResult *result,
    const BallisticsTrajectorySample *sample);
BallisticsStatus ballistics_simulation_result_set_stop(
    BallisticsSimulationResult *result,
    BallisticsStopReason reason,
    double time_s,
    const BallisticsProjectileState *state);

#endif
