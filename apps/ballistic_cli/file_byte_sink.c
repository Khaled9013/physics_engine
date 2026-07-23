#include "file_byte_sink.h"

#include <stddef.h>

static BallisticsStatus file_sink_write(void *context, const void *data, size_t size)
{
    FILE *file = context;
    if (file == NULL || (data == NULL && size != 0U))
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    return fwrite(data, 1U, size, file) == size ? BALLISTICS_STATUS_OK
                                                : BALLISTICS_STATUS_IO_ERROR;
}

static BallisticsStatus file_sink_flush(void *context)
{
    FILE *file = context;
    if (file == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    return fflush(file) == 0 ? BALLISTICS_STATUS_OK : BALLISTICS_STATUS_IO_ERROR;
}

BallisticsByteSink ballistics_cli_file_byte_sink(FILE *file)
{
    return (BallisticsByteSink){file_sink_write, file_sink_flush, file};
}
