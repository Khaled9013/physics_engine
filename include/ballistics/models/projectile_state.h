#ifndef BALLISTICS_MODELS_PROJECTILE_STATE_H
#define BALLISTICS_MODELS_PROJECTILE_STATE_H

#include "ballistics/export.h"
#include "ballistics/math/vector3.h"
#include "ballistics/status.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Point-mass translational state in the shared coordinate system. */
typedef struct
{
    BallisticsVector3 position_m;
    BallisticsVector3 velocity_mps;
} BallisticsProjectileState;

/** Time derivative of a projectile state. */
typedef struct
{
    BallisticsVector3 position_mps;
    BallisticsVector3 velocity_mps2;
} BallisticsStateDerivative;

BALLISTICS_API BallisticsStatus
ballistics_projectile_state_validate(const BallisticsProjectileState *state);

#ifdef __cplusplus
}
#endif

#endif
