#ifndef BALLISTICS_STOP_CONDITIONS_GROUND_STOP_CONDITION_H
#define BALLISTICS_STOP_CONDITIONS_GROUND_STOP_CONDITION_H

#include "ballistics/interfaces/stop_condition_interface.h"

typedef struct
{
    double ground_height_m;
} BallisticsGroundStopConfig;

BALLISTICS_API BallisticsStatus ballistics_ground_stop_condition_create(
    const BallisticsGroundStopConfig *config,
    BallisticsStopCondition **out_condition);

#endif
