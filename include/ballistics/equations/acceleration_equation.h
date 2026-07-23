#ifndef BALLISTICS_EQUATIONS_ACCELERATION_EQUATION_H
#define BALLISTICS_EQUATIONS_ACCELERATION_EQUATION_H

#include "ballistics/interfaces/equation_interface.h"
#include "ballistics/math/vector3.h"

#define BALLISTICS_ACCELERATION_EQUATION_ID "acceleration-from-force.v1"

typedef struct
{
    BallisticsVector3 total_force_n;
    double mass_kg;
} BallisticsAccelerationInput;

BALLISTICS_API BallisticsStatus ballistics_acceleration_evaluate(
    const BallisticsAccelerationInput *input,
    BallisticsVector3 *out_acceleration_mps2);
BALLISTICS_API BallisticsStatus
ballistics_acceleration_equation_create(BallisticsEquation **out_equation);
BALLISTICS_API BallisticsStatus ballistics_acceleration_equation_factory(
    const void *config, size_t config_size, BallisticsEquation **out_equation);

#endif
