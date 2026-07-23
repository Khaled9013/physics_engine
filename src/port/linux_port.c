#define _POSIX_C_SOURCE 200809L

#include "ballistics/port/ballistics_port.h"

#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

struct BallisticsPortMutex
{
    unsigned char reserved;
};

BallisticsStatus ballistics_port_monotonic_time(double *out_seconds)
{
    struct timespec value;

    if (out_seconds == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    if (clock_gettime(CLOCK_MONOTONIC, &value) != 0)
    {
        return BALLISTICS_STATUS_INTERNAL_ERROR;
    }
    *out_seconds = (double)value.tv_sec + ((double)value.tv_nsec * 1.0e-9);
    return BALLISTICS_STATUS_OK;
}

BallisticsStatus ballistics_port_allocate(size_t size, void **out_memory)
{
    if (out_memory == NULL || size == 0U)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    *out_memory = malloc(size);
    if (*out_memory == NULL)
    {
        return BALLISTICS_STATUS_OUT_OF_MEMORY;
    }
    return BALLISTICS_STATUS_OK;
}

BallisticsStatus ballistics_port_deallocate(void *memory)
{
    free(memory);
    return BALLISTICS_STATUS_OK;
}

BallisticsStatus ballistics_port_debug_write(const void *data, size_t size)
{
    const unsigned char *cursor = data;
    size_t remaining = size;

    if (data == NULL && size != 0U)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    while (remaining > 0U)
    {
        const ssize_t written = write(STDERR_FILENO, cursor, remaining);
        if (written < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            return BALLISTICS_STATUS_IO_ERROR;
        }
        if (written == 0)
        {
            return BALLISTICS_STATUS_IO_ERROR;
        }
        cursor += (size_t)written;
        remaining -= (size_t)written;
    }
    return BALLISTICS_STATUS_OK;
}

BallisticsStatus ballistics_port_mutex_create(BallisticsPortMutex **out_mutex)
{
    void *memory = NULL;
    BallisticsStatus status;

    if (out_mutex == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    *out_mutex = NULL;
    status = ballistics_port_allocate(sizeof(BallisticsPortMutex), &memory);
    if (status != BALLISTICS_STATUS_OK)
    {
        return status;
    }
    *out_mutex = memory;
    return BALLISTICS_STATUS_OK;
}

BallisticsStatus ballistics_port_mutex_lock(BallisticsPortMutex *mutex)
{
    return mutex == NULL ? BALLISTICS_STATUS_INVALID_ARGUMENT : BALLISTICS_STATUS_OK;
}

BallisticsStatus ballistics_port_mutex_unlock(BallisticsPortMutex *mutex)
{
    return mutex == NULL ? BALLISTICS_STATUS_INVALID_ARGUMENT : BALLISTICS_STATUS_OK;
}

BallisticsStatus ballistics_port_mutex_destroy(BallisticsPortMutex *mutex)
{
    return ballistics_port_deallocate(mutex);
}
