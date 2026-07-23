#include "ballistics/models/environment_state.h"

#include <math.h>
#include <stddef.h>

BallisticsStatus ballistics_environment_state_validate(const BallisticsEnvironmentState *state)
{
    if (state == NULL || !isfinite(state->air_density_kgpm3) || state->air_density_kgpm3 < 0.0 ||
        !ballistics_vector3_is_finite(&state->wind_velocity_mps))
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    return BALLISTICS_STATUS_OK;
}
