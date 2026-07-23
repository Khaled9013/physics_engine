#include "ballistics/models/projectile.h"

#include <math.h>
#include <stddef.h>

BallisticsStatus ballistics_projectile_validate(const BallisticsProjectile *projectile)
{
    if (projectile == NULL || !isfinite(projectile->mass_kg) || projectile->mass_kg <= 0.0 ||
        !isfinite(projectile->diameter_m) || projectile->diameter_m < 0.0 ||
        !isfinite(projectile->reference_area_m2) || projectile->reference_area_m2 <= 0.0)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    return BALLISTICS_STATUS_OK;
}
