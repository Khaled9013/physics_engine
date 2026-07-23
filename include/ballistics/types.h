#ifndef BALLISTICS_TYPES_H
#define BALLISTICS_TYPES_H

#include "ballistics/export.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Stable reason recorded when a simulation terminates. Values are append-only. */
typedef enum
{
    BALLISTICS_STOP_REASON_NONE = 0,
    BALLISTICS_STOP_REASON_MAXIMUM_TIME,
    BALLISTICS_STOP_REASON_GROUND_INTERSECTION,
    BALLISTICS_STOP_REASON_MAXIMUM_DISTANCE,
    BALLISTICS_STOP_REASON_INVALID_STATE
} BallisticsStopReason;

/** Categories are append-only; existing numeric values are never changed. */
typedef enum
{
    BALLISTICS_EQUATION_CATEGORY_KINEMATICS = 0,
    BALLISTICS_EQUATION_CATEGORY_AERODYNAMICS,
    BALLISTICS_EQUATION_CATEGORY_ATMOSPHERE,
    BALLISTICS_EQUATION_CATEGORY_CUSTOM
} BallisticsEquationCategory;

/** Opaque reserved mutex storage used only through the port hooks. */
typedef struct BallisticsPortMutex BallisticsPortMutex;

#ifdef __cplusplus
}
#endif

#endif
