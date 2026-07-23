#ifndef BALLISTICS_MODELS_LAUNCH_PROFILE_H
#define BALLISTICS_MODELS_LAUNCH_PROFILE_H

#include "ballistics/export.h"
#include "ballistics/math/vector3.h"
#include "ballistics/models/projectile_state.h"
#include "ballistics/status.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Generic launch state; direction is normalized when converted to state. */
typedef struct
{
    BallisticsVector3 initial_position_m;
    BallisticsVector3 direction;
    double initial_speed_mps;
} BallisticsLaunchState;

/** Reserved serializable launcher/sight metadata ignored by the Phase One solver. */
typedef struct
{
    const char *profile_identifier;
    const char *display_name;
    double sight_height_m;
    double bore_angle_rad;
} BallisticsLauncherMetadata;

BALLISTICS_API BallisticsStatus
ballistics_launch_state_validate(const BallisticsLaunchState *launch_state);
BALLISTICS_API BallisticsStatus ballistics_launch_state_to_projectile_state(
    const BallisticsLaunchState *launch_state,
    BallisticsProjectileState *out_state);
BALLISTICS_API BallisticsStatus ballistics_launcher_metadata_validate(
    const BallisticsLauncherMetadata *metadata);

#ifdef __cplusplus
}
#endif

#endif
