#include "ballistics/equations/air_relative_velocity_equation.h"

#include "ballistics/port/ballistics_port.h"

#include <stddef.h>

static BallisticsStatus air_relative_velocity_generic_evaluate(
    const BallisticsEquation *self, const void *input, size_t input_size,
    void *output, size_t output_size)
{
    (void)self;
    if (input == NULL || output == NULL || input_size != sizeof(BallisticsAirRelativeVelocityInput) ||
        output_size != sizeof(BallisticsVector3))
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    return ballistics_air_relative_velocity_evaluate(input, output);
}

static void air_relative_velocity_destroy(BallisticsEquation *equation)
{
    (void)ballistics_port_deallocate(equation);
}

static const BallisticsEquationVTable air_relative_velocity_vtable = {
    air_relative_velocity_generic_evaluate,
    NULL,
    air_relative_velocity_destroy,
};

BallisticsStatus ballistics_air_relative_velocity_equation_create(BallisticsEquation **out_equation)
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
    equation->vtable = &air_relative_velocity_vtable;
    equation->context = NULL;
    equation->identifier = BALLISTICS_AIR_RELATIVE_VELOCITY_EQUATION_ID;
    equation->name = "Air-relative velocity";
    equation->category = BALLISTICS_EQUATION_CATEGORY_KINEMATICS;
    equation->input_description = "projectile velocity and wind velocity";
    equation->output_description = "air-relative velocity";
    *out_equation = equation;
    return BALLISTICS_STATUS_OK;
}

BallisticsStatus ballistics_air_relative_velocity_equation_factory(
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
    return ballistics_air_relative_velocity_equation_create(out_equation);
}

BallisticsStatus ballistics_air_relative_velocity_evaluate(
    const BallisticsAirRelativeVelocityInput *input,
    BallisticsVector3 *out_relative_velocity_mps)
{
    if (input == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    return ballistics_vector3_subtract(&input->projectile_velocity_mps,
                                       &input->wind_velocity_mps,
                                       out_relative_velocity_mps);
}
