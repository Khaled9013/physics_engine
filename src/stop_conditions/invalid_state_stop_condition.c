#include "ballistics/stop_conditions/invalid_state_stop_condition.h"

#include "ballistics/port/ballistics_port.h"

#include <stddef.h>

static BallisticsStatus invalid_state_evaluate(const BallisticsStopCondition *self,
                                               const BallisticsStopEvaluation *evaluation,
                                               BallisticsStopDecision *out_decision)
{
    (void)self;
    if (ballistics_projectile_state_validate(evaluation->current_state) != BALLISTICS_STATUS_OK)
    {
        if (ballistics_projectile_state_validate(evaluation->previous_state) != BALLISTICS_STATUS_OK)
        {
            return BALLISTICS_STATUS_NUMERICAL_ERROR;
        }
        out_decision->stop = true;
        out_decision->reason = BALLISTICS_STOP_REASON_INVALID_STATE;
        out_decision->final_time_s = evaluation->previous_time_s;
        out_decision->final_state = *evaluation->previous_state;
    }
    return BALLISTICS_STATUS_OK;
}

static void invalid_state_destroy(BallisticsStopCondition *condition)
{
    (void)ballistics_port_deallocate(condition);
}

static const BallisticsStopConditionVTable invalid_state_vtable = {
    invalid_state_evaluate, invalid_state_destroy};

BallisticsStatus ballistics_invalid_state_stop_condition_create(
    const BallisticsInvalidStateStopConfig *config,
    BallisticsStopCondition **out_condition)
{
    BallisticsStopCondition *condition = NULL;
    void *memory = NULL;
    BallisticsStatus status;

    if (out_condition == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    *out_condition = NULL;
    if (config == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    status = ballistics_port_allocate(sizeof(*condition), &memory);
    if (status != BALLISTICS_STATUS_OK)
    {
        return status;
    }
    condition = memory;
    condition->vtable = &invalid_state_vtable;
    condition->context = NULL;
    condition->name = "invalid-state.v1";
    *out_condition = condition;
    return BALLISTICS_STATUS_OK;
}
