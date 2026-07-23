#include "ballistics/models/constant_gravity_model.h"

#include "ballistics/port/ballistics_port.h"

#include <stddef.h>

typedef struct
{
    BallisticsVector3 acceleration_mps2;
} BallisticsConstantGravityContext;

static BallisticsStatus constant_gravity_calculate_force(
    const BallisticsForceModel *self,
    const BallisticsProjectile *projectile,
    const BallisticsProjectileState *projectile_state,
    const BallisticsEnvironmentState *environment_state,
    BallisticsVector3 *out_force_n)
{
    const BallisticsConstantGravityContext *context;

    (void)projectile_state;
    (void)environment_state;
    if (self == NULL || self->context == NULL || projectile == NULL || out_force_n == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    context = self->context;
    return ballistics_vector3_scale(&context->acceleration_mps2, projectile->mass_kg, out_force_n);
}

static void constant_gravity_destroy(BallisticsForceModel *model)
{
    if (model != NULL)
    {
        (void)ballistics_port_deallocate(model->context);
        (void)ballistics_port_deallocate(model);
    }
}

static const BallisticsForceModelVTable constant_gravity_vtable = {
    constant_gravity_calculate_force,
    constant_gravity_destroy,
};

BallisticsStatus ballistics_constant_gravity_model_create(
    const BallisticsConstantGravityConfig *config,
    BallisticsGravityModel **out_model)
{
    BallisticsForceModel *model = NULL;
    BallisticsConstantGravityContext *context = NULL;
    void *memory = NULL;
    BallisticsStatus status;

    if (out_model == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    *out_model = NULL;
    if (config == NULL || !ballistics_vector3_is_finite(&config->acceleration_mps2))
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
    context->acceleration_mps2 = config->acceleration_mps2;
    model->vtable = &constant_gravity_vtable;
    model->context = context;
    model->name = BALLISTICS_CONSTANT_GRAVITY_MODEL_ID;
    *out_model = model;
    return BALLISTICS_STATUS_OK;
}

BallisticsStatus ballistics_constant_gravity_model_factory(
    const void *config, size_t config_size, BallisticsForceModel **out_model)
{
    static const BallisticsConstantGravityConfig defaults = {{0.0, 0.0, -9.80665}};

    if (config == NULL && config_size == 0U)
    {
        return ballistics_constant_gravity_model_create(&defaults, out_model);
    }
    if (config == NULL || config_size != sizeof(BallisticsConstantGravityConfig))
    {
        if (out_model != NULL)
        {
            *out_model = NULL;
        }
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    return ballistics_constant_gravity_model_create(config, out_model);
}
