#ifndef BALLISTICS_REGISTRY_WRITER_REGISTRY_H
#define BALLISTICS_REGISTRY_WRITER_REGISTRY_H

#include "ballistics/export.h"
#include "ballistics/status.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BallisticsTrajectoryWriter BallisticsTrajectoryWriter;
typedef struct BallisticsWriterRegistry BallisticsWriterRegistry;
typedef BallisticsStatus (*BallisticsWriterFactory)(
    const void *config, size_t config_size, BallisticsTrajectoryWriter **out_object);

/** Create an empty registry with fixed entry capacity. The caller owns it. */
BALLISTICS_API BallisticsStatus ballistics_writer_registry_create(
    size_t capacity, BallisticsWriterRegistry **out_registry);
BALLISTICS_API void ballistics_writer_registry_destroy(BallisticsWriterRegistry *registry);

/** Register a borrowed stable identifier and factory. Not thread-safe with mutation. */
BALLISTICS_API BallisticsStatus ballistics_writer_registry_register(
    BallisticsWriterRegistry *registry, const char *identifier, BallisticsWriterFactory factory);
BALLISTICS_API BallisticsStatus ballistics_writer_registry_find(
    const BallisticsWriterRegistry *registry, const char *identifier, BallisticsWriterFactory *out_factory);
BALLISTICS_API BallisticsStatus ballistics_writer_registry_count(
    const BallisticsWriterRegistry *registry, size_t *out_count);
BALLISTICS_API BallisticsStatus ballistics_writer_registry_entry_identifier(
    const BallisticsWriterRegistry *registry, size_t index, const char **out_identifier);
BALLISTICS_API BallisticsStatus ballistics_writer_registry_create_instance(
    const BallisticsWriterRegistry *registry, const char *identifier,
    const void *config, size_t config_size, BallisticsTrajectoryWriter **out_object);

#ifdef __cplusplus
}
#endif

#endif
