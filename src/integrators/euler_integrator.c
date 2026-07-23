#include "ballistics/integrators/euler_integrator.h"

#include "ballistics/port/ballistics_port.h"

#include <math.h>
#include <stddef.h>

typedef struct
{
    size_t maximum_state_count;
} BallisticsEulerContext;

static BallisticsStatus euler_step(const BallisticsIntegrator *self,
                                   const double *current_state,
                                   size_t state_count,
                                   double current_time_s,
                                   double time_step_s,
                                   BallisticsDerivativeFunction derivative,
                                   void *derivative_context,
                                   double *out_state)
{
    const BallisticsEulerContext *context;
    double state_derivative[BALLISTICS_EULER_MAXIMUM_STATE_COUNT];
    size_t index;
    BallisticsStatus status;

    if (self == NULL || self->context == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    context = self->context;
    if (state_count > context->maximum_state_count)
    {
        return BALLISTICS_STATUS_CAPACITY_EXCEEDED;
    }
    status = derivative(current_time_s,
                        current_state,
                        state_count,
                        state_derivative,
                        derivative_context);
    if (status != BALLISTICS_STATUS_OK)
    {
        return status;
    }
    for (index = 0U; index < state_count; ++index)
    {
        if (!isfinite(state_derivative[index]))
        {
            return BALLISTICS_STATUS_NUMERICAL_ERROR;
        }
        out_state[index] = current_state[index] + time_step_s * state_derivative[index];
    }
    return BALLISTICS_STATUS_OK;
}

static void euler_destroy(BallisticsIntegrator *integrator)
{
    if (integrator != NULL)
    {
        (void)ballistics_port_deallocate(integrator->context);
        (void)ballistics_port_deallocate(integrator);
    }
}

static const BallisticsIntegratorVTable euler_vtable = {euler_step, euler_destroy};

BallisticsStatus ballistics_euler_integrator_create(const BallisticsEulerIntegratorConfig *config,
                                                    BallisticsIntegrator **out_integrator)
{
    BallisticsIntegrator *integrator = NULL;
    BallisticsEulerContext *context = NULL;
    void *memory = NULL;
    BallisticsStatus status;

    if (out_integrator == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    *out_integrator = NULL;
    if (config == NULL || config->maximum_state_count == 0U ||
        config->maximum_state_count > BALLISTICS_EULER_MAXIMUM_STATE_COUNT)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    status = ballistics_port_allocate(sizeof(*integrator), &memory);
    if (status != BALLISTICS_STATUS_OK)
    {
        return status;
    }
    integrator = memory;
    status = ballistics_port_allocate(sizeof(*context), &memory);
    if (status != BALLISTICS_STATUS_OK)
    {
        (void)ballistics_port_deallocate(integrator);
        return status;
    }
    context = memory;
    context->maximum_state_count = config->maximum_state_count;
    integrator->vtable = &euler_vtable;
    integrator->context = context;
    integrator->name = BALLISTICS_EULER_INTEGRATOR_ID;
    *out_integrator = integrator;
    return BALLISTICS_STATUS_OK;
}

BallisticsStatus ballistics_euler_integrator_factory(
    const void *config, size_t config_size, BallisticsIntegrator **out_integrator)
{
    static const BallisticsEulerIntegratorConfig defaults = {
        BALLISTICS_EULER_MAXIMUM_STATE_COUNT};

    if (config == NULL && config_size == 0U)
    {
        return ballistics_euler_integrator_create(&defaults, out_integrator);
    }
    if (config == NULL || config_size != sizeof(BallisticsEulerIntegratorConfig))
    {
        if (out_integrator != NULL)
        {
            *out_integrator = NULL;
        }
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    return ballistics_euler_integrator_create(config, out_integrator);
}
