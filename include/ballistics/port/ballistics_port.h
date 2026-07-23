#ifndef BALLISTICS_PORT_BALLISTICS_PORT_H
#define BALLISTICS_PORT_BALLISTICS_PORT_H

#include "ballistics/export.h"
#include "ballistics/status.h"
#include "ballistics/types.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Read monotonic seconds. Thread-safe; caller owns the output scalar. */
BALLISTICS_API BallisticsStatus ballistics_port_monotonic_time(double *out_seconds);

/** Allocate size bytes and return owned storage through out_memory. Thread-safe. */
BALLISTICS_API BallisticsStatus ballistics_port_allocate(size_t size, void **out_memory);

/** Release storage returned by ballistics_port_allocate. NULL is accepted. */
BALLISTICS_API BallisticsStatus ballistics_port_deallocate(void *memory);

/** Write diagnostic bytes to the platform debug channel. No ownership transfer. */
BALLISTICS_API BallisticsStatus ballistics_port_debug_write(const void *data, size_t size);

/** Reserved synchronization hooks. Phase One does not require core locking. */
BALLISTICS_API BallisticsStatus ballistics_port_mutex_create(BallisticsPortMutex **out_mutex);
BALLISTICS_API BallisticsStatus ballistics_port_mutex_lock(BallisticsPortMutex *mutex);
BALLISTICS_API BallisticsStatus ballistics_port_mutex_unlock(BallisticsPortMutex *mutex);
BALLISTICS_API BallisticsStatus ballistics_port_mutex_destroy(BallisticsPortMutex *mutex);

#ifdef __cplusplus
}
#endif

#endif
