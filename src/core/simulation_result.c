#include "ballistics/simulation_result.h"

#include "ballistics/port/ballistics_port.h"
#include "simulation_result_internal.h"

#include <math.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

struct BallisticsSimulationResult
{
    BallisticsTrajectorySample *samples;
    size_t count;
    size_t capacity;
    BallisticsStopReason stop_reason;
    double final_time_s;
    BallisticsProjectileState final_state;
};

BallisticsStatus ballistics_simulation_result_create(size_t initial_capacity,
                                                     BallisticsSimulationResult **out_result)
{
    BallisticsSimulationResult *result = NULL;
    void *memory = NULL;
    BallisticsStatus status;

    if (out_result == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    *out_result = NULL;
    if (initial_capacity == 0U || initial_capacity > SIZE_MAX / sizeof(BallisticsTrajectorySample))
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    status = ballistics_port_allocate(sizeof(*result), &memory);
    if (status != BALLISTICS_STATUS_OK)
    {
        return status;
    }
    result = memory;
    status = ballistics_port_allocate(initial_capacity * sizeof(*result->samples), &memory);
    if (status != BALLISTICS_STATUS_OK)
    {
        (void)ballistics_port_deallocate(result);
        return status;
    }
    result->samples = memory;
    result->capacity = initial_capacity;
    result->count = 0U;
    result->stop_reason = BALLISTICS_STOP_REASON_NONE;
    result->final_time_s = 0.0;
    result->final_state = (BallisticsProjectileState){{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}};
    *out_result = result;
    return BALLISTICS_STATUS_OK;
}

void ballistics_simulation_result_destroy(BallisticsSimulationResult *result)
{
    if (result != NULL)
    {
        (void)ballistics_port_deallocate(result->samples);
        (void)ballistics_port_deallocate(result);
    }
}

BallisticsStatus ballistics_simulation_result_clear(BallisticsSimulationResult *result)
{
    if (result == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    result->count = 0U;
    result->stop_reason = BALLISTICS_STOP_REASON_NONE;
    result->final_time_s = 0.0;
    return BALLISTICS_STATUS_OK;
}

BallisticsStatus ballistics_simulation_result_append(BallisticsSimulationResult *result,
                                                     const BallisticsTrajectorySample *sample)
{
    if (result == NULL || sample == NULL || !isfinite(sample->time_s) ||
        ballistics_projectile_state_validate(&sample->state) != BALLISTICS_STATUS_OK ||
        !ballistics_vector3_is_finite(&sample->acceleration_mps2))
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    if (result->count == result->capacity)
    {
        BallisticsTrajectorySample *new_samples;
        size_t new_capacity;
        void *memory = NULL;
        BallisticsStatus status;

        if (result->capacity > SIZE_MAX / 2U)
        {
            return BALLISTICS_STATUS_CAPACITY_EXCEEDED;
        }
        new_capacity = result->capacity * 2U;
        if (new_capacity > SIZE_MAX / sizeof(*new_samples))
        {
            return BALLISTICS_STATUS_CAPACITY_EXCEEDED;
        }
        status = ballistics_port_allocate(new_capacity * sizeof(*new_samples), &memory);
        if (status != BALLISTICS_STATUS_OK)
        {
            return status;
        }
        new_samples = memory;
        memcpy(new_samples, result->samples, result->count * sizeof(*new_samples));
        (void)ballistics_port_deallocate(result->samples);
        result->samples = new_samples;
        result->capacity = new_capacity;
    }
    result->samples[result->count] = *sample;
    ++result->count;
    return BALLISTICS_STATUS_OK;
}

BallisticsStatus ballistics_simulation_result_set_stop(BallisticsSimulationResult *result,
                                                       BallisticsStopReason reason,
                                                       double time_s,
                                                       const BallisticsProjectileState *state)
{
    if (result == NULL || reason == BALLISTICS_STOP_REASON_NONE || !isfinite(time_s) ||
        ballistics_projectile_state_validate(state) != BALLISTICS_STATUS_OK)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    result->stop_reason = reason;
    result->final_time_s = time_s;
    result->final_state = *state;
    return BALLISTICS_STATUS_OK;
}

BallisticsStatus ballistics_simulation_result_sample_count(const BallisticsSimulationResult *result,
                                                           size_t *out_count)
{
    if (result == NULL || out_count == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    *out_count = result->count;
    return BALLISTICS_STATUS_OK;
}

BallisticsStatus ballistics_simulation_result_sample_at(const BallisticsSimulationResult *result,
                                                        size_t index,
                                                        const BallisticsTrajectorySample **out_sample)
{
    if (result == NULL || out_sample == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    *out_sample = NULL;
    if (index >= result->count)
    {
        return BALLISTICS_STATUS_NOT_FOUND;
    }
    *out_sample = &result->samples[index];
    return BALLISTICS_STATUS_OK;
}

BallisticsStatus ballistics_simulation_result_stop_reason(const BallisticsSimulationResult *result,
                                                          BallisticsStopReason *out_reason)
{
    if (result == NULL || out_reason == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    *out_reason = result->stop_reason;
    return BALLISTICS_STATUS_OK;
}

BallisticsStatus ballistics_simulation_result_final_time(const BallisticsSimulationResult *result,
                                                         double *out_time_s)
{
    if (result == NULL || out_time_s == NULL || result->stop_reason == BALLISTICS_STOP_REASON_NONE)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    *out_time_s = result->final_time_s;
    return BALLISTICS_STATUS_OK;
}

BallisticsStatus ballistics_simulation_result_final_state(const BallisticsSimulationResult *result,
                                                          BallisticsProjectileState *out_state)
{
    if (result == NULL || out_state == NULL || result->stop_reason == BALLISTICS_STOP_REASON_NONE)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    *out_state = result->final_state;
    return BALLISTICS_STATUS_OK;
}
