#include "ballistics/interfaces/stop_condition_interface.h"

#include <math.h>
#include <stddef.h>

BallisticsStatus ballistics_stop_condition_evaluate(const BallisticsStopCondition *condition,
                                                    const BallisticsStopEvaluation *evaluation,
                                                    BallisticsStopDecision *out_decision)
{
    BallisticsStatus status;

    if (condition == NULL || condition->vtable == NULL || condition->vtable->evaluate == NULL ||
        evaluation == NULL || evaluation->previous_state == NULL ||
        evaluation->current_state == NULL || evaluation->initial_state == NULL ||
        out_decision == NULL || !isfinite(evaluation->previous_time_s) ||
        !isfinite(evaluation->current_time_s) ||
        evaluation->current_time_s < evaluation->previous_time_s)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    out_decision->stop = false;
    out_decision->reason = BALLISTICS_STOP_REASON_NONE;
    status = condition->vtable->evaluate(condition, evaluation, out_decision);
    if (status != BALLISTICS_STATUS_OK || !out_decision->stop)
    {
        return status;
    }
    if (out_decision->reason == BALLISTICS_STOP_REASON_NONE ||
        !isfinite(out_decision->final_time_s) ||
        ballistics_projectile_state_validate(&out_decision->final_state) != BALLISTICS_STATUS_OK)
    {
        return BALLISTICS_STATUS_NUMERICAL_ERROR;
    }
    return BALLISTICS_STATUS_OK;
}

void ballistics_stop_condition_destroy(BallisticsStopCondition *condition)
{
    if (condition != NULL && condition->vtable != NULL && condition->vtable->destroy != NULL)
    {
        condition->vtable->destroy(condition);
    }
}
