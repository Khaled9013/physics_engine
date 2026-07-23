#ifndef BALLISTICS_INTERFACES_ENVIRONMENT_INTERFACE_H
#define BALLISTICS_INTERFACES_ENVIRONMENT_INTERFACE_H

#include "ballistics/export.h"
#include "ballistics/math/vector3.h"
#include "ballistics/models/environment_state.h"
#include "ballistics/status.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BallisticsEnvironmentModel BallisticsEnvironmentModel;
typedef struct
{
    BallisticsStatus (*sample)(const BallisticsEnvironmentModel *self,
                               double time_s,
                               const BallisticsVector3 *position_m,
                               BallisticsEnvironmentState *out_state);
    void (*destroy)(BallisticsEnvironmentModel *self);
} BallisticsEnvironmentModelVTable;

struct BallisticsEnvironmentModel
{
    const BallisticsEnvironmentModelVTable *vtable;
    void *context;
    const char *name;
};

BALLISTICS_API BallisticsStatus ballistics_environment_sample(
    const BallisticsEnvironmentModel *model,
    double time_s,
    const BallisticsVector3 *position_m,
    BallisticsEnvironmentState *out_state);
BALLISTICS_API void ballistics_environment_destroy(BallisticsEnvironmentModel *model);

#ifdef __cplusplus
}
#endif

#endif
