#ifndef BALLISTICS_INTERFACES_EQUATION_INTERFACE_H
#define BALLISTICS_INTERFACES_EQUATION_INTERFACE_H

#include "ballistics/export.h"
#include "ballistics/status.h"
#include "ballistics/types.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BallisticsEquation BallisticsEquation;

typedef struct
{
    BallisticsStatus (*evaluate)(const BallisticsEquation *self,
                                 const void *input,
                                 size_t input_size,
                                 void *output,
                                 size_t output_size);
    BallisticsStatus (*initialize)(BallisticsEquation *self);
    void (*destroy)(BallisticsEquation *self);
} BallisticsEquationVTable;

/** Common metadata and size-checked adapter for a typed pure equation. */
struct BallisticsEquation
{
    const BallisticsEquationVTable *vtable;
    void *context;
    const char *identifier;
    const char *name;
    BallisticsEquationCategory category;
    const char *input_description;
    const char *output_description;
};

BALLISTICS_API BallisticsStatus ballistics_equation_initialize(BallisticsEquation *equation);
BALLISTICS_API BallisticsStatus ballistics_equation_evaluate(const BallisticsEquation *equation,
                                                             const void *input,
                                                             size_t input_size,
                                                             void *output,
                                                             size_t output_size);
BALLISTICS_API void ballistics_equation_destroy(BallisticsEquation *equation);

#ifdef __cplusplus
}
#endif

#endif
