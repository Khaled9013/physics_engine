#ifndef BALLISTICS_STATUS_H
#define BALLISTICS_STATUS_H

#include "ballistics/export.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Status returned by every fallible public operation. Values are append-only. */
typedef enum
{
    BALLISTICS_STATUS_OK = 0,
    BALLISTICS_STATUS_INVALID_ARGUMENT,
    BALLISTICS_STATUS_NOT_INITIALIZED,
    BALLISTICS_STATUS_ALREADY_INITIALIZED,
    BALLISTICS_STATUS_NOT_FOUND,
    BALLISTICS_STATUS_DUPLICATE,
    BALLISTICS_STATUS_OUT_OF_MEMORY,
    BALLISTICS_STATUS_IO_ERROR,
    BALLISTICS_STATUS_NUMERICAL_ERROR,
    BALLISTICS_STATUS_UNSUPPORTED_PLATFORM,
    BALLISTICS_STATUS_CAPACITY_EXCEEDED,
    BALLISTICS_STATUS_INTERNAL_ERROR
} BallisticsStatus;

/** Return a static, human-readable name for a status value. Thread-safe. */
BALLISTICS_API const char *ballistics_status_to_string(BallisticsStatus status);

#ifdef __cplusplus
}
#endif

#endif
