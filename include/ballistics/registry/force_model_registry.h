#ifndef BALLISTICS_REGISTRY_FORCE_MODEL_REGISTRY_H
#define BALLISTICS_REGISTRY_FORCE_MODEL_REGISTRY_H

#include "ballistics/export.h"
#include "ballistics/status.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BallisticsForceModel BallisticsForceModel;
typedef struct BallisticsForceModelRegistry BallisticsForceModelRegistry;
typedef BallisticsStatus (*BallisticsForceModelFactory)(
    const void *config, size_t config_size, BallisticsForceModel **out_object);

/** Create an empty registry with fixed entry capacity. The caller owns it. */
BALLISTICS_API BallisticsStatus ballistics_force_model_registry_create(
    size_t capacity, BallisticsForceModelRegistry **out_registry);
BALLISTICS_API void ballistics_force_model_registry_destroy(BallisticsForceModelRegistry *registry);

/** Register a borrowed stable identifier and factory. Not thread-safe with mutation. */
BALLISTICS_API BallisticsStatus ballistics_force_model_registry_register(
    BallisticsForceModelRegistry *registry, const char *identifier, BallisticsForceModelFactory factory);
BALLISTICS_API BallisticsStatus ballistics_force_model_registry_find(
    const BallisticsForceModelRegistry *registry, const char *identifier, BallisticsForceModelFactory *out_factory);
BALLISTICS_API BallisticsStatus ballistics_force_model_registry_count(
    const BallisticsForceModelRegistry *registry, size_t *out_count);
BALLISTICS_API BallisticsStatus ballistics_force_model_registry_entry_identifier(
    const BallisticsForceModelRegistry *registry, size_t index, const char **out_identifier);
BALLISTICS_API BallisticsStatus ballistics_force_model_registry_create_instance(
    const BallisticsForceModelRegistry *registry, const char *identifier,
    const void *config, size_t config_size, BallisticsForceModel **out_object);

#ifdef __cplusplus
}
#endif

#endif
