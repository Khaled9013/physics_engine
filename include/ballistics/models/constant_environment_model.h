#ifndef BALLISTICS_MODELS_CONSTANT_ENVIRONMENT_MODEL_H
#define BALLISTICS_MODELS_CONSTANT_ENVIRONMENT_MODEL_H

#include "ballistics/interfaces/environment_interface.h"

typedef struct
{
    double air_density_kgpm3;
    BallisticsVector3 wind_velocity_mps;
} BallisticsConstantEnvironmentConfig;

/** Create an owned immutable constant-environment model from a copied config. */
BALLISTICS_API BallisticsStatus ballistics_constant_environment_model_create(
    const BallisticsConstantEnvironmentConfig *config,
    BallisticsEnvironmentModel **out_model);

#endif
