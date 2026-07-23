#include "ballistics/stop_conditions/maximum_time_stop_condition.h"

#include "ballistics/port/ballistics_port.h"

#include <math.h>
#include <stddef.h>

typedef struct
{
    double maximum_time_s;
} MaximumTimeContext;

static BallisticsVector3 interpolate_vector(BallisticsVector3 start,
                                            BallisticsVector3 end,
                                            double alpha)
{
    return (BallisticsVector3){start.x + alpha * (end.x - start.x),
                              start.y + alpha * (end.y - start.y),
                              start.z + alpha * (end.z - start.z)};
}

static BallisticsStatus maximum_time_evaluate(const BallisticsStopCondition *self,
                                              const BallisticsStopEvaluation *evaluation,
                                              BallisticsStopDecision *out_decision)
{
    const MaximumTimeContext *context = self->context;
    double alpha;

    if (context == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    if (evaluation->current_time_s < context->maximum_time_s)
    {
        return BALLISTICS_STATUS_OK;
    }
    if (evaluation->previous_time_s >= context->maximum_time_s)
    {
        alpha = 0.0;
    }
    else
    {
        alpha = (context->maximum_time_s - evaluation->previous_time_s) /
                (evaluation->current_time_s - evaluation->previous_time_s);
    }
    out_decision->stop = true;
    out_decision->reason = BALLISTICS_STOP_REASON_MAXIMUM_TIME;
    out_decision->final_time_s = context->maximum_time_s;
    out_decision->final_state.position_m = interpolate_vector(
        evaluation->previous_state->position_m, evaluation->current_state->position_m, alpha);
    out_decision->final_state.velocity_mps = interpolate_vector(
        evaluation->previous_state->velocity_mps, evaluation->current_state->velocity_mps, alpha);
    return BALLISTICS_STATUS_OK;
}

static void maximum_time_destroy(BallisticsStopCondition *condition)
{
    if (condition != NULL)
    {
        (void)ballistics_port_deallocate(condition->context);
        (void)ballistics_port_deallocate(condition);
    }
}

static const BallisticsStopConditionVTable maximum_time_vtable = {
    maximum_time_evaluate, maximum_time_destroy};

BallisticsStatus ballistics_maximum_time_stop_condition_create(
    const BallisticsMaximumTimeStopConfig *config,
    BallisticsStopCondition **out_condition)
{
    BallisticsStopCondition *condition = NULL;
    MaximumTimeContext *context = NULL;
    void *memory = NULL;
    BallisticsStatus status;

    if (out_condition == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    *out_condition = NULL;
    if (config == NULL || !isfinite(config->maximum_time_s) || config->maximum_time_s <= 0.0)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    status = ballistics_port_allocate(sizeof(*condition), &memory);
    if (status != BALLISTICS_STATUS_OK)
    {
        return status;
    }
    condition = memory;
    status = ballistics_port_allocate(sizeof(*context), &memory);
    if (status != BALLISTICS_STATUS_OK)
    {
        (void)ballistics_port_deallocate(condition);
        return status;
    }
    context = memory;
    context->maximum_time_s = config->maximum_time_s;
    condition->vtable = &maximum_time_vtable;
    condition->context = context;
    condition->name = "maximum-time.v1";
    *out_condition = condition;
    return BALLISTICS_STATUS_OK;
}
