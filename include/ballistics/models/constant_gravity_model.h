#ifndef BALLISTICS_MODELS_CONSTANT_GRAVITY_MODEL_H
#define BALLISTICS_MODELS_CONSTANT_GRAVITY_MODEL_H

#include "ballistics/interfaces/gravity_model_interface.h"
#include "ballistics/registry/force_model_registry.h"

#define BALLISTICS_CONSTANT_GRAVITY_MODEL_ID "constant-gravity.v1"

typedef struct
{
    BallisticsVector3 acceleration_mps2;
} BallisticsConstantGravityConfig;

BALLISTICS_API BallisticsStatus ballistics_constant_gravity_model_create(
    const BallisticsConstantGravityConfig *config,
    BallisticsGravityModel **out_model);
BALLISTICS_API BallisticsStatus ballistics_constant_gravity_model_factory(
    const void *config, size_t config_size, BallisticsForceModel **out_model);

#endif
