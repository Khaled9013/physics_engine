#ifndef BALLISTICS_INTERFACES_BYTE_SINK_H
#define BALLISTICS_INTERFACES_BYTE_SINK_H

#include "ballistics/status.h"
#include <stddef.h>

typedef struct
{
    BallisticsStatus (*write)(void *context, const void *data, size_t size);
    BallisticsStatus (*flush)(void *context);
    void *context;
} BallisticsByteSink;

#endif
