#ifndef BALLISTICS_INTERFACES_FORCE_MODEL_INTERFACE_H
#define BALLISTICS_INTERFACES_FORCE_MODEL_INTERFACE_H

#include "ballistics/export.h"
#include "ballistics/math/vector3.h"
#include "ballistics/models/environment_state.h"
#include "ballistics/models/projectile.h"
#include "ballistics/models/projectile_state.h"
#include "ballistics/status.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BallisticsForceModel BallisticsForceModel;
typedef struct
{
    BallisticsStatus (*calculate_force)(const BallisticsForceModel *self,
                                        const BallisticsProjectile *projectile,
                                        const BallisticsProjectileState *projectile_state,
                                        const BallisticsEnvironmentState *environment_state,
                                        BallisticsVector3 *out_force_n);
    void (*destroy)(BallisticsForceModel *self);
} BallisticsForceModelVTable;

struct BallisticsForceModel
{
    const BallisticsForceModelVTable *vtable;
    void *context;
    const char *name;
};

BALLISTICS_API BallisticsStatus ballistics_force_model_calculate_force(
    const BallisticsForceModel *model,
    const BallisticsProjectile *projectile,
    const BallisticsProjectileState *projectile_state,
    const BallisticsEnvironmentState *environment_state,
    BallisticsVector3 *out_force_n);
BALLISTICS_API void ballistics_force_model_destroy(BallisticsForceModel *model);

#ifdef __cplusplus
}
#endif

#endif
