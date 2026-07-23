#ifndef BALLISTICS_REGISTRY_EQUATION_REGISTRY_H
#define BALLISTICS_REGISTRY_EQUATION_REGISTRY_H

#include "ballistics/export.h"
#include "ballistics/status.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BallisticsEquation BallisticsEquation;
typedef struct BallisticsEquationRegistry BallisticsEquationRegistry;
typedef BallisticsStatus (*BallisticsEquationFactory)(
    const void *config, size_t config_size, BallisticsEquation **out_object);

/** Create an empty registry with fixed entry capacity. The caller owns it. */
BALLISTICS_API BallisticsStatus ballistics_equation_registry_create(
    size_t capacity, BallisticsEquationRegistry **out_registry);
BALLISTICS_API void ballistics_equation_registry_destroy(BallisticsEquationRegistry *registry);

/** Register a borrowed stable identifier and factory. Not thread-safe with mutation. */
BALLISTICS_API BallisticsStatus ballistics_equation_registry_register(
    BallisticsEquationRegistry *registry, const char *identifier, BallisticsEquationFactory factory);
BALLISTICS_API BallisticsStatus ballistics_equation_registry_find(
    const BallisticsEquationRegistry *registry, const char *identifier, BallisticsEquationFactory *out_factory);
BALLISTICS_API BallisticsStatus ballistics_equation_registry_count(
    const BallisticsEquationRegistry *registry, size_t *out_count);
BALLISTICS_API BallisticsStatus ballistics_equation_registry_entry_identifier(
    const BallisticsEquationRegistry *registry, size_t index, const char **out_identifier);
BALLISTICS_API BallisticsStatus ballistics_equation_registry_create_instance(
    const BallisticsEquationRegistry *registry, const char *identifier,
    const void *config, size_t config_size, BallisticsEquation **out_object);

#ifdef __cplusplus
}
#endif

#endif
