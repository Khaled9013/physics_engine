#include "ballistics/port/ballistics_port.h"

BallisticsStatus ballistics_port_monotonic_time(double *out_seconds)
{
    (void)out_seconds;
    return BALLISTICS_STATUS_UNSUPPORTED_PLATFORM;
}

BallisticsStatus ballistics_port_allocate(size_t size, void **out_memory)
{
    (void)size;
    if (out_memory != NULL)
    {
        *out_memory = NULL;
    }
    return BALLISTICS_STATUS_UNSUPPORTED_PLATFORM;
}

BallisticsStatus ballistics_port_deallocate(void *memory)
{
    (void)memory;
    return BALLISTICS_STATUS_UNSUPPORTED_PLATFORM;
}

BallisticsStatus ballistics_port_debug_write(const void *data, size_t size)
{
    (void)data;
    (void)size;
    return BALLISTICS_STATUS_UNSUPPORTED_PLATFORM;
}

BallisticsStatus ballistics_port_mutex_create(BallisticsPortMutex **out_mutex)
{
    if (out_mutex != NULL)
    {
        *out_mutex = NULL;
    }
    return BALLISTICS_STATUS_UNSUPPORTED_PLATFORM;
}

BallisticsStatus ballistics_port_mutex_lock(BallisticsPortMutex *mutex)
{
    (void)mutex;
    return BALLISTICS_STATUS_UNSUPPORTED_PLATFORM;
}

BallisticsStatus ballistics_port_mutex_unlock(BallisticsPortMutex *mutex)
{
    (void)mutex;
    return BALLISTICS_STATUS_UNSUPPORTED_PLATFORM;
}

BallisticsStatus ballistics_port_mutex_destroy(BallisticsPortMutex *mutex)
{
    (void)mutex;
    return BALLISTICS_STATUS_UNSUPPORTED_PLATFORM;
}
