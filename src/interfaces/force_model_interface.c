#include "ballistics/interfaces/force_model_interface.h"

#include <stddef.h>

BallisticsStatus ballistics_force_model_calculate_force(
    const BallisticsForceModel *model,
    const BallisticsProjectile *projectile,
    const BallisticsProjectileState *projectile_state,
    const BallisticsEnvironmentState *environment_state,
    BallisticsVector3 *out_force_n)
{
    BallisticsStatus status;

    if (model == NULL || model->vtable == NULL || model->vtable->calculate_force == NULL ||
        out_force_n == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    status = ballistics_projectile_validate(projectile);
    if (status != BALLISTICS_STATUS_OK)
    {
        return status;
    }
    status = ballistics_projectile_state_validate(projectile_state);
    if (status != BALLISTICS_STATUS_OK)
    {
        return status;
    }
    status = ballistics_environment_state_validate(environment_state);
    if (status != BALLISTICS_STATUS_OK)
    {
        return status;
    }
    status = model->vtable->calculate_force(
        model, projectile, projectile_state, environment_state, out_force_n);
    if (status == BALLISTICS_STATUS_OK && !ballistics_vector3_is_finite(out_force_n))
    {
        return BALLISTICS_STATUS_NUMERICAL_ERROR;
    }
    return status;
}

void ballistics_force_model_destroy(BallisticsForceModel *model)
{
    if (model != NULL && model->vtable != NULL && model->vtable->destroy != NULL)
    {
        model->vtable->destroy(model);
    }
}
