#ifndef BALLISTICS_SIMULATION_RESULT_H
#define BALLISTICS_SIMULATION_RESULT_H

#include "ballistics/export.h"
#include "ballistics/math/vector3.h"
#include "ballistics/models/projectile_state.h"
#include "ballistics/status.h"
#include "ballistics/types.h"
#include <stddef.h>

typedef struct
{
    double time_s;
    BallisticsProjectileState state;
    BallisticsVector3 acceleration_mps2;
} BallisticsTrajectorySample;

typedef struct BallisticsSimulationResult BallisticsSimulationResult;

BALLISTICS_API BallisticsStatus ballistics_simulation_result_create(
    size_t initial_capacity,
    BallisticsSimulationResult **out_result);
BALLISTICS_API void ballistics_simulation_result_destroy(BallisticsSimulationResult *result);
BALLISTICS_API BallisticsStatus ballistics_simulation_result_clear(BallisticsSimulationResult *result);
BALLISTICS_API BallisticsStatus ballistics_simulation_result_sample_count(
    const BallisticsSimulationResult *result,
    size_t *out_count);
/** Returned sample pointer is borrowed until clear, destroy, or the next run. */
BALLISTICS_API BallisticsStatus ballistics_simulation_result_sample_at(
    const BallisticsSimulationResult *result,
    size_t index,
    const BallisticsTrajectorySample **out_sample);
BALLISTICS_API BallisticsStatus ballistics_simulation_result_stop_reason(
    const BallisticsSimulationResult *result,
    BallisticsStopReason *out_reason);
BALLISTICS_API BallisticsStatus ballistics_simulation_result_final_time(
    const BallisticsSimulationResult *result,
    double *out_time_s);
BALLISTICS_API BallisticsStatus ballistics_simulation_result_final_state(
    const BallisticsSimulationResult *result,
    BallisticsProjectileState *out_state);

#endif
