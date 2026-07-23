#include "test_memory_sink.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static BallisticsStatus memory_write(void *context, const void *data, size_t size)
{
    BallisticsTestMemorySink *sink = context;
    size_t required;

    if (sink == NULL || (data == NULL && size != 0U) || size > SIZE_MAX - sink->size)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    required = sink->size + size;
    if (required > sink->capacity)
    {
        size_t new_capacity = sink->capacity == 0U ? 256U : sink->capacity;
        unsigned char *new_data;
        while (new_capacity < required)
        {
            if (new_capacity > SIZE_MAX / 2U)
            {
                return BALLISTICS_STATUS_CAPACITY_EXCEEDED;
            }
            new_capacity *= 2U;
        }
        new_data = realloc(sink->data, new_capacity);
        if (new_data == NULL)
        {
            return BALLISTICS_STATUS_OUT_OF_MEMORY;
        }
        sink->data = new_data;
        sink->capacity = new_capacity;
    }
    memcpy(sink->data + sink->size, data, size);
    sink->size = required;
    return BALLISTICS_STATUS_OK;
}

static BallisticsStatus memory_flush(void *context)
{
    return context == NULL ? BALLISTICS_STATUS_INVALID_ARGUMENT : BALLISTICS_STATUS_OK;
}

void ballistics_test_memory_sink_init(BallisticsTestMemorySink *sink)
{
    if (sink != NULL)
    {
        *sink = (BallisticsTestMemorySink){NULL, 0U, 0U};
    }
}

void ballistics_test_memory_sink_destroy(BallisticsTestMemorySink *sink)
{
    if (sink != NULL)
    {
        free(sink->data);
        *sink = (BallisticsTestMemorySink){NULL, 0U, 0U};
    }
}

BallisticsByteSink ballistics_test_memory_sink_interface(BallisticsTestMemorySink *sink)
{
    return (BallisticsByteSink){memory_write, memory_flush, sink};
}
