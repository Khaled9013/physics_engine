#ifndef BALLISTICS_MODELS_PROJECTILE_H
#define BALLISTICS_MODELS_PROJECTILE_H

#include "ballistics/export.h"
#include "ballistics/status.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Generic projectile properties in SI units. */
typedef struct
{
    double mass_kg;
    double diameter_m;
    double reference_area_m2;
} BallisticsProjectile;

/** Validate finite physical projectile properties. Thread-safe. */
BALLISTICS_API BallisticsStatus ballistics_projectile_validate(const BallisticsProjectile *projectile);

#ifdef __cplusplus
}
#endif

#endif
