#include "ballistics/equations/acceleration_equation.h"

#include "ballistics/port/ballistics_port.h"

#include <stddef.h>

static BallisticsStatus acceleration_generic_evaluate(
    const BallisticsEquation *self, const void *input, size_t input_size,
    void *output, size_t output_size)
{
    (void)self;
    if (input == NULL || output == NULL || input_size != sizeof(BallisticsAccelerationInput) ||
        output_size != sizeof(BallisticsVector3))
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    return ballistics_acceleration_evaluate(input, output);
}

static void acceleration_destroy(BallisticsEquation *equation)
{
    (void)ballistics_port_deallocate(equation);
}

static const BallisticsEquationVTable acceleration_vtable = {
    acceleration_generic_evaluate,
    NULL,
    acceleration_destroy,
};

BallisticsStatus ballistics_acceleration_equation_create(BallisticsEquation **out_equation)
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
    equation->vtable = &acceleration_vtable;
    equation->context = NULL;
    equation->identifier = BALLISTICS_ACCELERATION_EQUATION_ID;
    equation->name = "Acceleration from force and mass";
    equation->category = BALLISTICS_EQUATION_CATEGORY_KINEMATICS;
    equation->input_description = "total force and mass";
    equation->output_description = "acceleration";
    *out_equation = equation;
    return BALLISTICS_STATUS_OK;
}

BallisticsStatus ballistics_acceleration_equation_factory(
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
    return ballistics_acceleration_equation_create(out_equation);
}

#include <math.h>

BallisticsStatus ballistics_acceleration_evaluate(
    const BallisticsAccelerationInput *input,
    BallisticsVector3 *out_acceleration_mps2)
{
    if (input == NULL || out_acceleration_mps2 == NULL ||
        !ballistics_vector3_is_finite(&input->total_force_n) ||
        !isfinite(input->mass_kg) || input->mass_kg <= 0.0)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    return ballistics_vector3_scale(&input->total_force_n,
                                     1.0 / input->mass_kg,
                                     out_acceleration_mps2);
}
