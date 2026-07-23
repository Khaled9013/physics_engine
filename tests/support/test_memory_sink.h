#ifndef BALLISTICS_TEST_MEMORY_SINK_H
#define BALLISTICS_TEST_MEMORY_SINK_H

#include "ballistics/interfaces/byte_sink.h"
#include <stddef.h>

typedef struct
{
    unsigned char *data;
    size_t size;
    size_t capacity;
} BallisticsTestMemorySink;

void ballistics_test_memory_sink_init(BallisticsTestMemorySink *sink);
void ballistics_test_memory_sink_destroy(BallisticsTestMemorySink *sink);
BallisticsByteSink ballistics_test_memory_sink_interface(BallisticsTestMemorySink *sink);

#endif
