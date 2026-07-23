#include "ballistics/interfaces/integrator_interface.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

static bool state_is_finite(const double *state, size_t state_count)
{
    size_t index;

    for (index = 0U; index < state_count; ++index)
    {
        if (!isfinite(state[index]))
        {
            return false;
        }
    }
    return true;
}

static bool ranges_overlap(const double *left, const double *right, size_t count)
{
    const uintptr_t left_start = (uintptr_t)left;
    const uintptr_t right_start = (uintptr_t)right;
    const size_t byte_count = count * sizeof(double);

    return left_start < right_start + byte_count && right_start < left_start + byte_count;
}

BallisticsStatus ballistics_integrator_step(const BallisticsIntegrator *integrator,
                                            const double *current_state,
                                            size_t state_count,
                                            double current_time_s,
                                            double time_step_s,
                                            BallisticsDerivativeFunction derivative,
                                            void *derivative_context,
                                            double *out_state)
{
    BallisticsStatus status;

    if (integrator == NULL || integrator->vtable == NULL || integrator->vtable->step == NULL ||
        current_state == NULL || state_count == 0U || out_state == NULL || derivative == NULL ||
        !isfinite(current_time_s) || !isfinite(time_step_s) || time_step_s <= 0.0 ||
        state_count > SIZE_MAX / sizeof(double) ||
        ranges_overlap(current_state, out_state, state_count) ||
        !state_is_finite(current_state, state_count))
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    status = integrator->vtable->step(integrator,
                                      current_state,
                                      state_count,
                                      current_time_s,
                                      time_step_s,
                                      derivative,
                                      derivative_context,
                                      out_state);
    if (status == BALLISTICS_STATUS_OK && !state_is_finite(out_state, state_count))
    {
        return BALLISTICS_STATUS_NUMERICAL_ERROR;
    }
    return status;
}

void ballistics_integrator_destroy(BallisticsIntegrator *integrator)
{
    if (integrator != NULL && integrator->vtable != NULL && integrator->vtable->destroy != NULL)
    {
        integrator->vtable->destroy(integrator);
    }
}
