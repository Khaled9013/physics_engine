#include "ballistics/equations/aerodynamic_drag_equation.h"

#include "ballistics/port/ballistics_port.h"

#include <stddef.h>

static BallisticsStatus aerodynamic_drag_generic_evaluate(
    const BallisticsEquation *self, const void *input, size_t input_size,
    void *output, size_t output_size)
{
    (void)self;
    if (input == NULL || output == NULL || input_size != sizeof(BallisticsAerodynamicDragInput) ||
        output_size != sizeof(BallisticsVector3))
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    return ballistics_aerodynamic_drag_evaluate(input, output);
}

static void aerodynamic_drag_destroy(BallisticsEquation *equation)
{
    (void)ballistics_port_deallocate(equation);
}

static const BallisticsEquationVTable aerodynamic_drag_vtable = {
    aerodynamic_drag_generic_evaluate,
    NULL,
    aerodynamic_drag_destroy,
};

BallisticsStatus ballistics_aerodynamic_drag_equation_create(BallisticsEquation **out_equation)
{
    BallisticsEquation *equation = NULL;
    void *memory = NULL;
    BallisticsStatus status;

    if (out_equation == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    *out_equation = NULL;
    status = ballistics_port_allocate(sizeof(*equation), &memory);
    if (status != BALLISTICS_STATUS_OK)
    {
        return status;
    }
    equation = memory;
    equation->vtable = &aerodynamic_drag_vtable;
    equation->context = NULL;
    equation->identifier = BALLISTICS_AERODYNAMIC_DRAG_EQUATION_ID;
    equation->name = "Basic aerodynamic drag";
    equation->category = BALLISTICS_EQUATION_CATEGORY_AERODYNAMICS;
    equation->input_description = "relative velocity, density, coefficient, area";
    equation->output_description = "drag force";
    *out_equation = equation;
    return BALLISTICS_STATUS_OK;
}

BallisticsStatus ballistics_aerodynamic_drag_equation_factory(
    const void *config, size_t config_size, BallisticsEquation **out_equation)
{
    if (config != NULL || config_size != 0U)
    {
        if (out_equation != NULL)
        {
            *out_equation = NULL;
        }
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    return ballistics_aerodynamic_drag_equation_create(out_equation);
}

#include <math.h>

BallisticsStatus ballistics_aerodynamic_drag_evaluate(
    const BallisticsAerodynamicDragInput *input,
    BallisticsVector3 *out_drag_force_n)
{
    double relative_speed_mps;
    double scale;
    BallisticsStatus status;

    if (input == NULL || out_drag_force_n == NULL ||
        !ballistics_vector3_is_finite(&input->relative_air_velocity_mps) ||
        !isfinite(input->air_density_kgpm3) || input->air_density_kgpm3 < 0.0 ||
        !isfinite(input->drag_coefficient) || input->drag_coefficient < 0.0 ||
        !isfinite(input->reference_area_m2) || input->reference_area_m2 <= 0.0)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    status = ballistics_vector3_magnitude(&input->relative_air_velocity_mps, &relative_speed_mps);
    if (status != BALLISTICS_STATUS_OK)
    {
        return status;
    }
    scale = -0.5 * input->air_density_kgpm3 * input->drag_coefficient *
            input->reference_area_m2 * relative_speed_mps;
    if (!isfinite(scale))
    {
        return BALLISTICS_STATUS_NUMERICAL_ERROR;
    }
    return ballistics_vector3_scale(&input->relative_air_velocity_mps, scale, out_drag_force_n);
}
