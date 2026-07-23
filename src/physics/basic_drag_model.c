#include "ballistics/models/basic_drag_model.h"

#include "ballistics/equations/aerodynamic_drag_equation.h"
#include "ballistics/equations/air_relative_velocity_equation.h"
#include "ballistics/port/ballistics_port.h"

#include <math.h>
#include <stddef.h>

typedef struct
{
    double drag_coefficient;
} BallisticsBasicDragContext;

static BallisticsStatus basic_drag_calculate_force(
    const BallisticsForceModel *self,
    const BallisticsProjectile *projectile,
    const BallisticsProjectileState *projectile_state,
    const BallisticsEnvironmentState *environment_state,
    BallisticsVector3 *out_force_n)
{
    const BallisticsBasicDragContext *context;
    BallisticsAirRelativeVelocityInput relative_input;
    BallisticsAerodynamicDragInput drag_input;
    BallisticsStatus status;

    if (self == NULL || self->context == NULL || projectile == NULL || projectile_state == NULL ||
        environment_state == NULL || out_force_n == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    context = self->context;
    relative_input.projectile_velocity_mps = projectile_state->velocity_mps;
    relative_input.wind_velocity_mps = environment_state->wind_velocity_mps;
    status = ballistics_air_relative_velocity_evaluate(
        &relative_input, &drag_input.relative_air_velocity_mps);
    if (status != BALLISTICS_STATUS_OK)
    {
        return status;
    }
    drag_input.air_density_kgpm3 = environment_state->air_density_kgpm3;
    drag_input.drag_coefficient = context->drag_coefficient;
    drag_input.reference_area_m2 = projectile->reference_area_m2;
    return ballistics_aerodynamic_drag_evaluate(&drag_input, out_force_n);
}

static void basic_drag_destroy(BallisticsForceModel *model)
{
    if (model != NULL)
    {
        (void)ballistics_port_deallocate(model->context);
        (void)ballistics_port_deallocate(model);
    }
}

static const BallisticsForceModelVTable basic_drag_vtable = {
    basic_drag_calculate_force,
    basic_drag_destroy,
};

BallisticsStatus ballistics_basic_drag_model_create(const BallisticsBasicDragConfig *config,
                                                    BallisticsDragModel **out_model)
{
    BallisticsForceModel *model = NULL;
    BallisticsBasicDragContext *context = NULL;
    void *memory = NULL;
    BallisticsStatus status;

    if (out_model == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    *out_model = NULL;
    if (config == NULL || !isfinite(config->drag_coefficient) || config->drag_coefficient < 0.0)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    status = ballistics_port_allocate(sizeof(*model), &memory);
    if (status != BALLISTICS_STATUS_OK)
    {
        return status;
    }
    model = memory;
    status = ballistics_port_allocate(sizeof(*context), &memory);
    if (status != BALLISTICS_STATUS_OK)
    {
        (void)ballistics_port_deallocate(model);
        return status;
    }
    context = memory;
    context->drag_coefficient = config->drag_coefficient;
    model->vtable = &basic_drag_vtable;
    model->context = context;
    model->name = BALLISTICS_BASIC_DRAG_MODEL_ID;
    *out_model = model;
    return BALLISTICS_STATUS_OK;
}

BallisticsStatus ballistics_basic_drag_model_factory(
    const void *config, size_t config_size, BallisticsForceModel **out_model)
{
    if (config == NULL || config_size != sizeof(BallisticsBasicDragConfig))
    {
        if (out_model != NULL)
        {
            *out_model = NULL;
        }
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    return ballistics_basic_drag_model_create(config, out_model);
}
