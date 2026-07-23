#include "ballistics/models/constant_environment_model.h"

#include "ballistics/port/ballistics_port.h"

#include <stddef.h>

typedef struct
{
    BallisticsEnvironmentState state;
} BallisticsConstantEnvironmentContext;

static BallisticsStatus constant_environment_sample(const BallisticsEnvironmentModel *self,
                                                     double time_s,
                                                     const BallisticsVector3 *position_m,
                                                     BallisticsEnvironmentState *out_state)
{
    const BallisticsConstantEnvironmentContext *context;

    (void)time_s;
    (void)position_m;
    if (self == NULL || self->context == NULL || out_state == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    context = self->context;
    *out_state = context->state;
    return BALLISTICS_STATUS_OK;
}

static void constant_environment_destroy(BallisticsEnvironmentModel *model)
{
    if (model != NULL)
    {
        (void)ballistics_port_deallocate(model->context);
        (void)ballistics_port_deallocate(model);
    }
}

static const BallisticsEnvironmentModelVTable constant_environment_vtable = {
    constant_environment_sample,
    constant_environment_destroy,
};

BallisticsStatus ballistics_constant_environment_model_create(
    const BallisticsConstantEnvironmentConfig *config,
    BallisticsEnvironmentModel **out_model)
{
    BallisticsEnvironmentModel *model = NULL;
    BallisticsConstantEnvironmentContext *context = NULL;
    BallisticsEnvironmentState state;
    void *memory = NULL;
    BallisticsStatus status;

    if (out_model == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    *out_model = NULL;
    if (config == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    state.air_density_kgpm3 = config->air_density_kgpm3;
    state.wind_velocity_mps = config->wind_velocity_mps;
    status = ballistics_environment_state_validate(&state);
    if (status != BALLISTICS_STATUS_OK)
    {
        return status;
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
    context->state = state;
    model->vtable = &constant_environment_vtable;
    model->context = context;
    model->name = "constant-environment.v1";
    *out_model = model;
    return BALLISTICS_STATUS_OK;
}
