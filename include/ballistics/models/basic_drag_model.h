#ifndef BALLISTICS_MODELS_BASIC_DRAG_MODEL_H
#define BALLISTICS_MODELS_BASIC_DRAG_MODEL_H

#include "ballistics/interfaces/drag_model_interface.h"
#include <stddef.h>

#define BALLISTICS_BASIC_DRAG_MODEL_ID "basic-drag.v1"

typedef struct
{
    double drag_coefficient;
} BallisticsBasicDragConfig;

BALLISTICS_API BallisticsStatus ballistics_basic_drag_model_create(
    const BallisticsBasicDragConfig *config,
    BallisticsDragModel **out_model);
BALLISTICS_API BallisticsStatus ballistics_basic_drag_model_factory(
    const void *config, size_t config_size, BallisticsForceModel **out_model);

#endif
