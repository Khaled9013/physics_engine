#ifndef BALLISTICS_STOP_CONDITIONS_MAXIMUM_DISTANCE_STOP_CONDITION_H
#define BALLISTICS_STOP_CONDITIONS_MAXIMUM_DISTANCE_STOP_CONDITION_H

#include "ballistics/interfaces/stop_condition_interface.h"

typedef struct
{
    double maximum_horizontal_distance_m;
} BallisticsMaximumDistanceStopConfig;

BALLISTICS_API BallisticsStatus ballistics_maximum_distance_stop_condition_create(
    const BallisticsMaximumDistanceStopConfig *config,
    BallisticsStopCondition **out_condition);

#endif
