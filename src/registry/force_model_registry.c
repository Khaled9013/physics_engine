#include "ballistics/registry/force_model_registry.h"

#include "ballistics/port/ballistics_port.h"
#include "registry_validation.h"

#include <stdint.h>
#include <stddef.h>
#include <string.h>

typedef struct
{
    const char *identifier;
    BallisticsForceModelFactory factory;
} BallisticsForceModelRegistryEntry;

struct BallisticsForceModelRegistry
{
    BallisticsForceModelRegistryEntry *entries;
    size_t count;
    size_t capacity;
};

BallisticsStatus ballistics_force_model_registry_create(
    size_t capacity, BallisticsForceModelRegistry **out_registry)
{
    BallisticsForceModelRegistry *registry = NULL;
    void *memory = NULL;
    BallisticsStatus status;

    if (out_registry == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    *out_registry = NULL;
    if (capacity == 0U ||
        capacity > SIZE_MAX / sizeof(BallisticsForceModelRegistryEntry))
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    status = ballistics_port_allocate(sizeof(*registry), &memory);
    if (status != BALLISTICS_STATUS_OK)
    {
        return status;
    }
    registry = memory;
    status = ballistics_port_allocate(capacity * sizeof(*registry->entries), &memory);
    if (status != BALLISTICS_STATUS_OK)
    {
        (void)ballistics_port_deallocate(registry);
        return status;
    }
    registry->entries = memory;
    registry->count = 0U;
    registry->capacity = capacity;
    *out_registry = registry;
    return BALLISTICS_STATUS_OK;
}

void ballistics_force_model_registry_destroy(BallisticsForceModelRegistry *registry)
{
    if (registry != NULL)
    {
        (void)ballistics_port_deallocate(registry->entries);
        (void)ballistics_port_deallocate(registry);
    }
}

BallisticsStatus ballistics_force_model_registry_register(
    BallisticsForceModelRegistry *registry, const char *identifier, BallisticsForceModelFactory factory)
{
    size_t index;

    if (registry == NULL || !ballistics_registry_identifier_is_valid(identifier) || factory == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    for (index = 0U; index < registry->count; ++index)
    {
        if (strcmp(registry->entries[index].identifier, identifier) == 0)
        {
            return BALLISTICS_STATUS_DUPLICATE;
        }
    }
    if (registry->count == registry->capacity)
    {
        return BALLISTICS_STATUS_CAPACITY_EXCEEDED;
    }
    registry->entries[registry->count].identifier = identifier;
    registry->entries[registry->count].factory = factory;
    ++registry->count;
    return BALLISTICS_STATUS_OK;
}

BallisticsStatus ballistics_force_model_registry_find(
    const BallisticsForceModelRegistry *registry, const char *identifier, BallisticsForceModelFactory *out_factory)
{
    size_t index;

    if (registry == NULL || !ballistics_registry_identifier_is_valid(identifier) ||
        out_factory == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    *out_factory = NULL;
    for (index = 0U; index < registry->count; ++index)
    {
        if (strcmp(registry->entries[index].identifier, identifier) == 0)
        {
            *out_factory = registry->entries[index].factory;
            return BALLISTICS_STATUS_OK;
        }
    }
    return BALLISTICS_STATUS_NOT_FOUND;
}

BallisticsStatus ballistics_force_model_registry_count(
    const BallisticsForceModelRegistry *registry, size_t *out_count)
{
    if (registry == NULL || out_count == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    *out_count = registry->count;
    return BALLISTICS_STATUS_OK;
}

BallisticsStatus ballistics_force_model_registry_entry_identifier(
    const BallisticsForceModelRegistry *registry, size_t index, const char **out_identifier)
{
    if (registry == NULL || out_identifier == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    *out_identifier = NULL;
    if (index >= registry->count)
    {
        return BALLISTICS_STATUS_NOT_FOUND;
    }
    *out_identifier = registry->entries[index].identifier;
    return BALLISTICS_STATUS_OK;
}

BallisticsStatus ballistics_force_model_registry_create_instance(
    const BallisticsForceModelRegistry *registry, const char *identifier,
    const void *config, size_t config_size, BallisticsForceModel **out_object)
{
    BallisticsForceModelFactory factory = NULL;
    BallisticsStatus status;

    if (out_object == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    *out_object = NULL;
    status = ballistics_force_model_registry_find(registry, identifier, &factory);
    if (status != BALLISTICS_STATUS_OK)
    {
        return status;
    }
    return factory(config, config_size, out_object);
}
