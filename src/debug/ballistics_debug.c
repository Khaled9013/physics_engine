#include "ballistics/debug/ballistics_debug.h"

#include "ballistics/port/ballistics_port.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define BALLISTICS_DEBUG_BUFFER_SIZE 1024U

typedef struct
{
    BallisticsDebugLevel level;
    BallisticsDebugOutputCallback callback;
    void *callback_context;
} BallisticsDebugConfiguration;

/* The debug configuration is the documented process-wide mutable exception. */
static BallisticsDebugConfiguration ballistics_debug_configuration = {
    BALLISTICS_DEBUG_LEVEL_WARNING,
    NULL,
    NULL,
};

static const char *ballistics_debug_level_name(BallisticsDebugLevel level)
{
    static const char *const names[] = {"ERROR", "WARNING", "INFO", "DEBUG", "TRACE"};
    return names[(size_t)level];
}

BallisticsStatus ballistics_debug_set_level(BallisticsDebugLevel level)
{
    if (level < BALLISTICS_DEBUG_LEVEL_ERROR || level > BALLISTICS_DEBUG_LEVEL_TRACE)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    ballistics_debug_configuration.level = level;
    return BALLISTICS_STATUS_OK;
}

BallisticsDebugLevel ballistics_debug_get_level(void)
{
    return ballistics_debug_configuration.level;
}

BallisticsStatus ballistics_debug_set_output(BallisticsDebugOutputCallback callback, void *context)
{
    if (callback == NULL && context != NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    ballistics_debug_configuration.callback = callback;
    ballistics_debug_configuration.callback_context = context;
    return BALLISTICS_STATUS_OK;
}

BallisticsStatus ballistics_debug_log(BallisticsDebugLevel level,
                                      const char *subsystem,
                                      const char *source_file,
                                      const char *function_name,
                                      int line,
                                      const char *format,
                                      ...)
{
    char buffer[BALLISTICS_DEBUG_BUFFER_SIZE];
    double timestamp_s;
    int prefix_length;
    int message_length;
    size_t output_size;
    va_list arguments;
    BallisticsStatus status;

    if (level < BALLISTICS_DEBUG_LEVEL_ERROR || level > BALLISTICS_DEBUG_LEVEL_TRACE ||
        subsystem == NULL || source_file == NULL || function_name == NULL || format == NULL ||
        line < 0)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    if (level > ballistics_debug_configuration.level)
    {
        return BALLISTICS_STATUS_OK;
    }
    status = ballistics_port_monotonic_time(&timestamp_s);
    if (status != BALLISTICS_STATUS_OK)
    {
        return status;
    }
    prefix_length = snprintf(buffer,
                             sizeof(buffer),
                             "[%.6f] [%s] [%s] %s:%d %s: ",
                             timestamp_s,
                             ballistics_debug_level_name(level),
                             subsystem,
                             source_file,
                             line,
                             function_name);
    if (prefix_length < 0 || (size_t)prefix_length >= sizeof(buffer))
    {
        return BALLISTICS_STATUS_INTERNAL_ERROR;
    }

    va_start(arguments, format);
    message_length = vsnprintf(buffer + (size_t)prefix_length,
                               sizeof(buffer) - (size_t)prefix_length,
                               format,
                               arguments);
    va_end(arguments);
    if (message_length < 0)
    {
        return BALLISTICS_STATUS_INTERNAL_ERROR;
    }
    output_size = (size_t)prefix_length + (size_t)message_length;
    if (output_size >= sizeof(buffer) - 1U)
    {
        output_size = sizeof(buffer) - 2U;
    }
    buffer[output_size] = '\n';
    ++output_size;

    if (ballistics_debug_configuration.callback != NULL)
    {
        return ballistics_debug_configuration.callback(
            buffer, output_size, ballistics_debug_configuration.callback_context);
    }
    return ballistics_port_debug_write(buffer, output_size);
}
