#ifndef BALLISTICS_INTERFACES_STOP_CONDITION_INTERFACE_H
#define BALLISTICS_INTERFACES_STOP_CONDITION_INTERFACE_H

#include "ballistics/export.h"
#include "ballistics/models/projectile_state.h"
#include "ballistics/status.h"
#include "ballistics/types.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    double previous_time_s;
    const BallisticsProjectileState *previous_state;
    double current_time_s;
    const BallisticsProjectileState *current_state;
    const BallisticsProjectileState *initial_state;
} BallisticsStopEvaluation;

typedef struct
{
    bool stop;
    BallisticsStopReason reason;
    double final_time_s;
    BallisticsProjectileState final_state;
} BallisticsStopDecision;

typedef struct BallisticsStopCondition BallisticsStopCondition;
typedef struct
{
    BallisticsStatus (*evaluate)(const BallisticsStopCondition *self,
                                 const BallisticsStopEvaluation *evaluation,
                                 BallisticsStopDecision *out_decision);
    void (*destroy)(BallisticsStopCondition *self);
} BallisticsStopConditionVTable;

struct BallisticsStopCondition
{
    const BallisticsStopConditionVTable *vtable;
    void *context;
    const char *name;
};

BALLISTICS_API BallisticsStatus ballistics_stop_condition_evaluate(
    const BallisticsStopCondition *condition,
    const BallisticsStopEvaluation *evaluation,
    BallisticsStopDecision *out_decision);
BALLISTICS_API void ballistics_stop_condition_destroy(BallisticsStopCondition *condition);

#ifdef __cplusplus
}
#endif

#endif
