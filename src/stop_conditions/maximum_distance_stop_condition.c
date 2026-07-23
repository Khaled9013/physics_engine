#include "ballistics/stop_conditions/maximum_distance_stop_condition.h"

#include "ballistics/port/ballistics_port.h"

#include <math.h>
#include <stddef.h>

typedef struct
{
    double maximum_horizontal_distance_m;
} MaximumDistanceContext;

static BallisticsStatus maximum_distance_evaluate(const BallisticsStopCondition *self,
                                                  const BallisticsStopEvaluation *evaluation,
                                                  BallisticsStopDecision *out_decision)
{
    const MaximumDistanceContext *context = self->context;
    double delta_x;
    double delta_y;
    double horizontal_distance_m;

    if (context == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    if (ballistics_projectile_state_validate(evaluation->current_state) != BALLISTICS_STATUS_OK ||
        ballistics_projectile_state_validate(evaluation->initial_state) != BALLISTICS_STATUS_OK)
    {
        return BALLISTICS_STATUS_OK;
    }
    delta_x = evaluation->current_state->position_m.x - evaluation->initial_state->position_m.x;
    delta_y = evaluation->current_state->position_m.y - evaluation->initial_state->position_m.y;
    horizontal_distance_m = hypot(delta_x, delta_y);
    if (!isfinite(horizontal_distance_m))
    {
        return BALLISTICS_STATUS_OK;
    }
    if (horizontal_distance_m >= context->maximum_horizontal_distance_m)
    {
        out_decision->stop = true;
        out_decision->reason = BALLISTICS_STOP_REASON_MAXIMUM_DISTANCE;
        out_decision->final_time_s = evaluation->current_time_s;
        out_decision->final_state = *evaluation->current_state;
    }
    return BALLISTICS_STATUS_OK;
}

static void maximum_distance_destroy(BallisticsStopCondition *condition)
{
    if (condition != NULL)
    {
        (void)ballistics_port_deallocate(condition->context);
        (void)ballistics_port_deallocate(condition);
    }
}

static const BallisticsStopConditionVTable maximum_distance_vtable = {
    maximum_distance_evaluate, maximum_distance_destroy};

BallisticsStatus ballistics_maximum_distance_stop_condition_create(
    const BallisticsMaximumDistanceStopConfig *config,
    BallisticsStopCondition **out_condition)
{
    BallisticsStopCondition *condition = NULL;
    MaximumDistanceContext *context = NULL;
    void *memory = NULL;
    BallisticsStatus status;

    if (out_condition == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    *out_condition = NULL;
    if (config == NULL || !isfinite(config->maximum_horizontal_distance_m) ||
        config->maximum_horizontal_distance_m <= 0.0)
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
    context->maximum_horizontal_distance_m = config->maximum_horizontal_distance_m;
    condition->vtable = &maximum_distance_vtable;
    condition->context = context;
    condition->name = "maximum-horizontal-distance.v1";
    *out_condition = condition;
    return BALLISTICS_STATUS_OK;
}
