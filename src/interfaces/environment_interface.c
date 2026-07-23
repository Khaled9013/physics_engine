#include "ballistics/interfaces/environment_interface.h"

#include <math.h>
#include <stddef.h>

BallisticsStatus ballistics_environment_sample(const BallisticsEnvironmentModel *model,
                                               double time_s,
                                               const BallisticsVector3 *position_m,
                                               BallisticsEnvironmentState *out_state)
{
    if (model == NULL || model->vtable == NULL || model->vtable->sample == NULL ||
        !isfinite(time_s) || !ballistics_vector3_is_finite(position_m) || out_state == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    return model->vtable->sample(model, time_s, position_m, out_state);
}

void ballistics_environment_destroy(BallisticsEnvironmentModel *model)
{
    if (model != NULL && model->vtable != NULL && model->vtable->destroy != NULL)
    {
        model->vtable->destroy(model);
    }
}
