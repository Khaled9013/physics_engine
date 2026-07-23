#ifndef BALLISTICS_DYNAMICS_H
#define BALLISTICS_DYNAMICS_H

#include "ballistics/export.h"
#include "ballistics/interfaces/environment_interface.h"
#include "ballistics/interfaces/force_model_interface.h"
#include "ballistics/models/projectile.h"
#include "ballistics/models/projectile_state.h"
#include "ballistics/status.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BallisticsDynamicsContext BallisticsDynamicsContext;

/** Create dynamics that borrows projectile, environment, model array, and models. */
BALLISTICS_API BallisticsStatus ballistics_dynamics_create(
    const BallisticsProjectile *projectile,
    const BallisticsEnvironmentModel *environment,
    BallisticsForceModel *const *force_models,
    size_t force_model_count,
    BallisticsDynamicsContext **out_dynamics);
BALLISTICS_API void ballistics_dynamics_destroy(BallisticsDynamicsContext *dynamics);

/** Evaluate projectile acceleration after ordered environment/force sampling. */
BALLISTICS_API BallisticsStatus ballistics_dynamics_acceleration(
    const BallisticsDynamicsContext *dynamics,
    double time_s,
    const BallisticsProjectileState *state,
    BallisticsVector3 *out_acceleration_mps2);

/** Generic six-value derivative adapter: [x,y,z,vx,vy,vz]. */
BALLISTICS_API BallisticsStatus ballistics_dynamics_derivative(
    double time_s,
    const double *state,
    size_t state_count,
    double *out_derivative,
    void *context);

#ifdef __cplusplus
}
#endif

#endif
