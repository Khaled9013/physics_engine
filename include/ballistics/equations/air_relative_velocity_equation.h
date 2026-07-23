#ifndef BALLISTICS_EQUATIONS_AIR_RELATIVE_VELOCITY_EQUATION_H
#define BALLISTICS_EQUATIONS_AIR_RELATIVE_VELOCITY_EQUATION_H

#include "ballistics/interfaces/equation_interface.h"
#include "ballistics/math/vector3.h"

#define BALLISTICS_AIR_RELATIVE_VELOCITY_EQUATION_ID "air-relative-velocity.v1"

typedef struct
{
    BallisticsVector3 projectile_velocity_mps;
    BallisticsVector3 wind_velocity_mps;
} BallisticsAirRelativeVelocityInput;

BALLISTICS_API BallisticsStatus ballistics_air_relative_velocity_evaluate(
    const BallisticsAirRelativeVelocityInput *input,
    BallisticsVector3 *out_relative_velocity_mps);
BALLISTICS_API BallisticsStatus
ballistics_air_relative_velocity_equation_create(BallisticsEquation **out_equation);
BALLISTICS_API BallisticsStatus ballistics_air_relative_velocity_equation_factory(
    const void *config, size_t config_size, BallisticsEquation **out_equation);

#endif
