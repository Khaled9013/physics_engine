#ifndef BALLISTICS_REGISTRY_INTEGRATOR_REGISTRY_H
#define BALLISTICS_REGISTRY_INTEGRATOR_REGISTRY_H

#include "ballistics/export.h"
#include "ballistics/status.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BallisticsIntegrator BallisticsIntegrator;
typedef struct BallisticsIntegratorRegistry BallisticsIntegratorRegistry;
typedef BallisticsStatus (*BallisticsIntegratorFactory)(
    const void *config, size_t config_size, BallisticsIntegrator **out_object);

/** Create an empty registry with fixed entry capacity. The caller owns it. */
BALLISTICS_API BallisticsStatus ballistics_integrator_registry_create(
    size_t capacity, BallisticsIntegratorRegistry **out_registry);
BALLISTICS_API void ballistics_integrator_registry_destroy(BallisticsIntegratorRegistry *registry);

/** Register a borrowed stable identifier and factory. Not thread-safe with mutation. */
BALLISTICS_API BallisticsStatus ballistics_integrator_registry_register(
    BallisticsIntegratorRegistry *registry, const char *identifier, BallisticsIntegratorFactory factory);
BALLISTICS_API BallisticsStatus ballistics_integrator_registry_find(
    const BallisticsIntegratorRegistry *registry, const char *identifier, BallisticsIntegratorFactory *out_factory);
BALLISTICS_API BallisticsStatus ballistics_integrator_registry_count(
    const BallisticsIntegratorRegistry *registry, size_t *out_count);
BALLISTICS_API BallisticsStatus ballistics_integrator_registry_entry_identifier(
    const BallisticsIntegratorRegistry *registry, size_t index, const char **out_identifier);
BALLISTICS_API BallisticsStatus ballistics_integrator_registry_create_instance(
    const BallisticsIntegratorRegistry *registry, const char *identifier,
    const void *config, size_t config_size, BallisticsIntegrator **out_object);

#ifdef __cplusplus
}
#endif

#endif
