#ifndef BALLISTICS_EQUATIONS_AERODYNAMIC_DRAG_EQUATION_H
#define BALLISTICS_EQUATIONS_AERODYNAMIC_DRAG_EQUATION_H

#include "ballistics/interfaces/equation_interface.h"
#include "ballistics/math/vector3.h"

#define BALLISTICS_AERODYNAMIC_DRAG_EQUATION_ID "basic-aerodynamic-drag.v1"

typedef struct
{
    BallisticsVector3 relative_air_velocity_mps;
    double air_density_kgpm3;
    double drag_coefficient;
    double reference_area_m2;
} BallisticsAerodynamicDragInput;

BALLISTICS_API BallisticsStatus ballistics_aerodynamic_drag_evaluate(
    const BallisticsAerodynamicDragInput *input,
    BallisticsVector3 *out_drag_force_n);
BALLISTICS_API BallisticsStatus
ballistics_aerodynamic_drag_equation_create(BallisticsEquation **out_equation);
BALLISTICS_API BallisticsStatus ballistics_aerodynamic_drag_equation_factory(
    const void *config, size_t config_size, BallisticsEquation **out_equation);

#endif
