#include "ballistics/models/projectile_state.h"

#include <stddef.h>

BallisticsStatus ballistics_projectile_state_validate(const BallisticsProjectileState *state)
{
    if (state == NULL || !ballistics_vector3_is_finite(&state->position_m) ||
        !ballistics_vector3_is_finite(&state->velocity_mps))
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    return BALLISTICS_STATUS_OK;
}
