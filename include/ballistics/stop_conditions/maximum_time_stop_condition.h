#ifndef BALLISTICS_STOP_CONDITIONS_MAXIMUM_TIME_STOP_CONDITION_H
#define BALLISTICS_STOP_CONDITIONS_MAXIMUM_TIME_STOP_CONDITION_H

#include "ballistics/interfaces/stop_condition_interface.h"

typedef struct
{
    double maximum_time_s;
} BallisticsMaximumTimeStopConfig;

BALLISTICS_API BallisticsStatus ballistics_maximum_time_stop_condition_create(
    const BallisticsMaximumTimeStopConfig *config,
    BallisticsStopCondition **out_condition);

#endif
