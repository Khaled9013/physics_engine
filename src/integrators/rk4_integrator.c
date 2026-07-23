#include "ballistics/integrators/rk4_integrator.h"

#include "ballistics/port/ballistics_port.h"

#include <math.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct
{
    size_t maximum_state_count;
} BallisticsRk4Context;

static bool derivative_is_finite(const double *values, size_t count)
{
    size_t index;
    for (index = 0U; index < count; ++index)
    {
        if (!isfinite(values[index]))
        {
            return false;
        }
    }
    return true;
}

static BallisticsStatus evaluate_stage(BallisticsDerivativeFunction derivative,
                                       double time_s,
                                       const double *state,
                                       size_t state_count,
                                       void *context,
                                       double *out_derivative)
{
    const BallisticsStatus status =
        derivative(time_s, state, state_count, out_derivative, context);
    if (status != BALLISTICS_STATUS_OK)
    {
        return status;
    }
    return derivative_is_finite(out_derivative, state_count) ? BALLISTICS_STATUS_OK
                                                              : BALLISTICS_STATUS_NUMERICAL_ERROR;
}

static BallisticsStatus rk4_step(const BallisticsIntegrator *self,
                                 const double *current_state,
                                 size_t state_count,
                                 double current_time_s,
                                 double time_step_s,
                                 BallisticsDerivativeFunction derivative,
                                 void *derivative_context,
                                 double *out_state)
{
    const BallisticsRk4Context *context;
    double k1[BALLISTICS_RK4_MAXIMUM_STATE_COUNT];
    double k2[BALLISTICS_RK4_MAXIMUM_STATE_COUNT];
    double k3[BALLISTICS_RK4_MAXIMUM_STATE_COUNT];
    double k4[BALLISTICS_RK4_MAXIMUM_STATE_COUNT];
    double temporary[BALLISTICS_RK4_MAXIMUM_STATE_COUNT];
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
    status = evaluate_stage(derivative,
                            current_time_s,
                            current_state,
                            state_count,
                            derivative_context,
                            k1);
    if (status != BALLISTICS_STATUS_OK)
    {
        return status;
    }
    for (index = 0U; index < state_count; ++index)
    {
        temporary[index] = current_state[index] + 0.5 * time_step_s * k1[index];
    }
    if (!derivative_is_finite(temporary, state_count))
    {
        return BALLISTICS_STATUS_NUMERICAL_ERROR;
    }
    status = evaluate_stage(derivative,
                            current_time_s + 0.5 * time_step_s,
                            temporary,
                            state_count,
                            derivative_context,
                            k2);
    if (status != BALLISTICS_STATUS_OK)
    {
        return status;
    }
    for (index = 0U; index < state_count; ++index)
    {
        temporary[index] = current_state[index] + 0.5 * time_step_s * k2[index];
    }
    if (!derivative_is_finite(temporary, state_count))
    {
        return BALLISTICS_STATUS_NUMERICAL_ERROR;
    }
    status = evaluate_stage(derivative,
                            current_time_s + 0.5 * time_step_s,
                            temporary,
                            state_count,
                            derivative_context,
                            k3);
    if (status != BALLISTICS_STATUS_OK)
    {
        return status;
    }
    for (index = 0U; index < state_count; ++index)
    {
        temporary[index] = current_state[index] + time_step_s * k3[index];
    }
    if (!derivative_is_finite(temporary, state_count))
    {
        return BALLISTICS_STATUS_NUMERICAL_ERROR;
    }
    status = evaluate_stage(derivative,
                            current_time_s + time_step_s,
                            temporary,
                            state_count,
                            derivative_context,
                            k4);
    if (status != BALLISTICS_STATUS_OK)
    {
        return status;
    }
    for (index = 0U; index < state_count; ++index)
    {
        out_state[index] = current_state[index] +
                           (time_step_s / 6.0) *
                               (k1[index] + 2.0 * k2[index] + 2.0 * k3[index] + k4[index]);
    }
    return BALLISTICS_STATUS_OK;
}

static void rk4_destroy(BallisticsIntegrator *integrator)
{
    if (integrator != NULL)
    {
        (void)ballistics_port_deallocate(integrator->context);
        (void)ballistics_port_deallocate(integrator);
    }
}

static const BallisticsIntegratorVTable rk4_vtable = {rk4_step, rk4_destroy};

BallisticsStatus ballistics_rk4_integrator_create(const BallisticsRk4IntegratorConfig *config,
                                                  BallisticsIntegrator **out_integrator)
{
    BallisticsIntegrator *integrator = NULL;
    BallisticsRk4Context *context = NULL;
    void *memory = NULL;
    BallisticsStatus status;

    if (out_integrator == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    *out_integrator = NULL;
    if (config == NULL || config->maximum_state_count == 0U ||
        config->maximum_state_count > BALLISTICS_RK4_MAXIMUM_STATE_COUNT)
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
    integrator->vtable = &rk4_vtable;
    integrator->context = context;
    integrator->name = BALLISTICS_RK4_INTEGRATOR_ID;
    *out_integrator = integrator;
    return BALLISTICS_STATUS_OK;
}

BallisticsStatus ballistics_rk4_integrator_factory(
    const void *config, size_t config_size, BallisticsIntegrator **out_integrator)
{
    static const BallisticsRk4IntegratorConfig defaults = {BALLISTICS_RK4_MAXIMUM_STATE_COUNT};

    if (config == NULL && config_size == 0U)
    {
        return ballistics_rk4_integrator_create(&defaults, out_integrator);
    }
    if (config == NULL || config_size != sizeof(BallisticsRk4IntegratorConfig))
    {
        if (out_integrator != NULL)
        {
            *out_integrator = NULL;
        }
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    return ballistics_rk4_integrator_create(config, out_integrator);
}
