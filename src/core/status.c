#include "ballistics/status.h"

const char *ballistics_status_to_string(BallisticsStatus status)
{
    switch (status)
    {
        case BALLISTICS_STATUS_OK:
            return "ok";
        case BALLISTICS_STATUS_INVALID_ARGUMENT:
            return "invalid argument";
        case BALLISTICS_STATUS_NOT_INITIALIZED:
            return "not initialized";
        case BALLISTICS_STATUS_ALREADY_INITIALIZED:
            return "already initialized";
        case BALLISTICS_STATUS_NOT_FOUND:
            return "not found";
        case BALLISTICS_STATUS_DUPLICATE:
            return "duplicate";
        case BALLISTICS_STATUS_OUT_OF_MEMORY:
            return "out of memory";
        case BALLISTICS_STATUS_IO_ERROR:
            return "I/O error";
        case BALLISTICS_STATUS_NUMERICAL_ERROR:
            return "numerical error";
        case BALLISTICS_STATUS_UNSUPPORTED_PLATFORM:
            return "unsupported platform";
        case BALLISTICS_STATUS_CAPACITY_EXCEEDED:
            return "capacity exceeded";
        case BALLISTICS_STATUS_INTERNAL_ERROR:
            return "internal error";
        default:
            return "unknown status";
    }
}
