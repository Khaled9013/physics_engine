#include "ballistics/stop_conditions/ground_stop_condition.h"

#include "ballistics/port/ballistics_port.h"

#include <math.h>
#include <stddef.h>

typedef struct
{
    double ground_height_m;
} GroundContext;

static BallisticsVector3 interpolate_vector(BallisticsVector3 start,
                                            BallisticsVector3 end,
                                            double alpha)
{
    return (BallisticsVector3){start.x + alpha * (end.x - start.x),
                              start.y + alpha * (end.y - start.y),
                              start.z + alpha * (end.z - start.z)};
}

static BallisticsStatus ground_evaluate(const BallisticsStopCondition *self,
                                        const BallisticsStopEvaluation *evaluation,
                                        BallisticsStopDecision *out_decision)
{
    const GroundContext *context = self->context;
    const double previous_z = evaluation->previous_state->position_m.z;
    const double current_z = evaluation->current_state->position_m.z;
    double alpha;

    if (context == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    if (ballistics_projectile_state_validate(evaluation->previous_state) != BALLISTICS_STATUS_OK ||
        ballistics_projectile_state_validate(evaluation->current_state) != BALLISTICS_STATUS_OK)
    {
        return BALLISTICS_STATUS_OK;
    }
    if (previous_z < context->ground_height_m)
    {
        out_decision->stop = true;
        out_decision->reason = BALLISTICS_STOP_REASON_GROUND_INTERSECTION;
        out_decision->final_time_s = evaluation->previous_time_s;
        out_decision->final_state = *evaluation->previous_state;
        out_decision->final_state.position_m.z = context->ground_height_m;
        return BALLISTICS_STATUS_OK;
    }
    if (!((previous_z > context->ground_height_m && current_z <= context->ground_height_m) ||
          (previous_z == context->ground_height_m && current_z < context->ground_height_m)))
    {
        return BALLISTICS_STATUS_OK;
    }

    /* Linear bracketing interpolation is deterministic and normally second-order in time.
       It is intentionally not a dense-output root solve. */
    alpha = (context->ground_height_m - previous_z) / (current_z - previous_z);
    alpha = fmax(0.0, fmin(1.0, alpha));
    out_decision->stop = true;
    out_decision->reason = BALLISTICS_STOP_REASON_GROUND_INTERSECTION;
    out_decision->final_time_s = evaluation->previous_time_s +
                                 alpha * (evaluation->current_time_s -
                                          evaluation->previous_time_s);
    out_decision->final_state.position_m = interpolate_vector(
        evaluation->previous_state->position_m, evaluation->current_state->position_m, alpha);
    out_decision->final_state.velocity_mps = interpolate_vector(
        evaluation->previous_state->velocity_mps, evaluation->current_state->velocity_mps, alpha);
    out_decision->final_state.position_m.z = context->ground_height_m;
    return BALLISTICS_STATUS_OK;
}

static void ground_destroy(BallisticsStopCondition *condition)
{
    if (condition != NULL)
    {
        (void)ballistics_port_deallocate(condition->context);
        (void)ballistics_port_deallocate(condition);
    }
}

static const BallisticsStopConditionVTable ground_vtable = {ground_evaluate, ground_destroy};

BallisticsStatus ballistics_ground_stop_condition_create(const BallisticsGroundStopConfig *config,
                                                         BallisticsStopCondition **out_condition)
{
    BallisticsStopCondition *condition = NULL;
    GroundContext *context = NULL;
    void *memory = NULL;
    BallisticsStatus status;

    if (out_condition == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    *out_condition = NULL;
    if (config == NULL || !isfinite(config->ground_height_m))
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
    context->ground_height_m = config->ground_height_m;
    condition->vtable = &ground_vtable;
    condition->context = context;
    condition->name = "ground-intersection.v1";
    *out_condition = condition;
    return BALLISTICS_STATUS_OK;
}
