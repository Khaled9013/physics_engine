#ifndef BALLISTICS_STOP_CONDITIONS_INVALID_STATE_STOP_CONDITION_H
#define BALLISTICS_STOP_CONDITIONS_INVALID_STATE_STOP_CONDITION_H

#include "ballistics/interfaces/stop_condition_interface.h"

typedef struct
{
    unsigned char reserved;
} BallisticsInvalidStateStopConfig;

BALLISTICS_API BallisticsStatus ballistics_invalid_state_stop_condition_create(
    const BallisticsInvalidStateStopConfig *config,
    BallisticsStopCondition **out_condition);

#endif
