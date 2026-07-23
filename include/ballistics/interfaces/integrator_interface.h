#ifndef BALLISTICS_INTERFACES_INTEGRATOR_INTERFACE_H
#define BALLISTICS_INTERFACES_INTEGRATOR_INTERFACE_H

#include "ballistics/export.h"
#include "ballistics/status.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef BallisticsStatus (*BallisticsDerivativeFunction)(double time_s,
                                                         const double *state,
                                                         size_t state_count,
                                                         double *out_derivative,
                                                         void *context);

typedef struct BallisticsIntegrator BallisticsIntegrator;
typedef struct
{
    BallisticsStatus (*step)(const BallisticsIntegrator *self,
                             const double *current_state,
                             size_t state_count,
                             double current_time_s,
                             double time_step_s,
                             BallisticsDerivativeFunction derivative,
                             void *derivative_context,
                             double *out_state);
    void (*destroy)(BallisticsIntegrator *self);
} BallisticsIntegratorVTable;

struct BallisticsIntegrator
{
    const BallisticsIntegratorVTable *vtable;
    void *context;
    const char *name;
};

/** Execute one fixed step. Inputs/output must be distinct non-overlapping arrays. */
BALLISTICS_API BallisticsStatus ballistics_integrator_step(
    const BallisticsIntegrator *integrator,
    const double *current_state,
    size_t state_count,
    double current_time_s,
    double time_step_s,
    BallisticsDerivativeFunction derivative,
    void *derivative_context,
    double *out_state);
BALLISTICS_API void ballistics_integrator_destroy(BallisticsIntegrator *integrator);

#ifdef __cplusplus
}
#endif

#endif
