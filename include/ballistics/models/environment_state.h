#ifndef BALLISTICS_MODELS_ENVIRONMENT_STATE_H
#define BALLISTICS_MODELS_ENVIRONMENT_STATE_H

#include "ballistics/export.h"
#include "ballistics/math/vector3.h"
#include "ballistics/status.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Atmosphere sampled at one trajectory location and time. */
typedef struct
{
    double air_density_kgpm3;
    BallisticsVector3 wind_velocity_mps;
} BallisticsEnvironmentState;

BALLISTICS_API BallisticsStatus
ballistics_environment_state_validate(const BallisticsEnvironmentState *state);

#ifdef __cplusplus
}
#endif

#endif
