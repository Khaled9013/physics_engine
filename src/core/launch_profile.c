#include "ballistics/models/launch_profile.h"

#include <math.h>
#include <stddef.h>

BallisticsStatus ballistics_launch_state_validate(const BallisticsLaunchState *launch_state)
{
    double direction_magnitude;
    BallisticsStatus status;

    if (launch_state == NULL || !ballistics_vector3_is_finite(&launch_state->initial_position_m) ||
        !ballistics_vector3_is_finite(&launch_state->direction) ||
        !isfinite(launch_state->initial_speed_mps) || launch_state->initial_speed_mps < 0.0)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    status = ballistics_vector3_magnitude(&launch_state->direction, &direction_magnitude);
    if (status != BALLISTICS_STATUS_OK || direction_magnitude == 0.0)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    return BALLISTICS_STATUS_OK;
}

BallisticsStatus ballistics_launch_state_to_projectile_state(
    const BallisticsLaunchState *launch_state,
    BallisticsProjectileState *out_state)
{
    BallisticsVector3 normalized_direction;
    BallisticsStatus status;

    if (out_state == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    status = ballistics_launch_state_validate(launch_state);
    if (status != BALLISTICS_STATUS_OK)
    {
        return status;
    }
    status = ballistics_vector3_normalize(&launch_state->direction, &normalized_direction);
    if (status != BALLISTICS_STATUS_OK)
    {
        return status;
    }
    out_state->position_m = launch_state->initial_position_m;
    return ballistics_vector3_scale(
        &normalized_direction, launch_state->initial_speed_mps, &out_state->velocity_mps);
}

BallisticsStatus ballistics_launcher_metadata_validate(const BallisticsLauncherMetadata *metadata)
{
    if (metadata == NULL || !isfinite(metadata->sight_height_m) ||
        !isfinite(metadata->bore_angle_rad))
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    return BALLISTICS_STATUS_OK;
}
