#ifndef BALLISTICS_DEBUG_BALLISTICS_DEBUG_H
#define BALLISTICS_DEBUG_BALLISTICS_DEBUG_H

#include "ballistics/export.h"
#include "ballistics/status.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    BALLISTICS_DEBUG_LEVEL_ERROR = 0,
    BALLISTICS_DEBUG_LEVEL_WARNING,
    BALLISTICS_DEBUG_LEVEL_INFO,
    BALLISTICS_DEBUG_LEVEL_DEBUG,
    BALLISTICS_DEBUG_LEVEL_TRACE
} BallisticsDebugLevel;

typedef BallisticsStatus (*BallisticsDebugOutputCallback)(const void *data,
                                                          size_t size,
                                                          void *context);

/** Configure the process-wide debug threshold. Configure before worker threads. */
BALLISTICS_API BallisticsStatus ballistics_debug_set_level(BallisticsDebugLevel level);
BALLISTICS_API BallisticsDebugLevel ballistics_debug_get_level(void);

/** Set an optional borrowed output callback/context; NULL restores the port backend. */
BALLISTICS_API BallisticsStatus ballistics_debug_set_output(BallisticsDebugOutputCallback callback,
                                                            void *context);

/** Format and emit one diagnostic record. Never used for simulation result data. */
BALLISTICS_API BallisticsStatus ballistics_debug_log(BallisticsDebugLevel level,
                                                     const char *subsystem,
                                                     const char *source_file,
                                                     const char *function_name,
                                                     int line,
                                                     const char *format,
                                                     ...);

#ifdef __cplusplus
}
#endif

#ifndef BALLISTICS_DEBUG_ENABLED
#define BALLISTICS_DEBUG_ENABLED 1
#endif

#if BALLISTICS_DEBUG_ENABLED
#define BALLISTICS_DEBUG_CALL(level_, subsystem_, ...)                                            \
    ((void)ballistics_debug_log((level_), (subsystem_), __FILE__, __func__, __LINE__, __VA_ARGS__))
#define BALLISTICS_DEBUG_ERROR(...)                                                               \
    BALLISTICS_DEBUG_CALL(BALLISTICS_DEBUG_LEVEL_ERROR, "core", __VA_ARGS__)
#define BALLISTICS_DEBUG_WARNING(...)                                                             \
    BALLISTICS_DEBUG_CALL(BALLISTICS_DEBUG_LEVEL_WARNING, "core", __VA_ARGS__)
#define BALLISTICS_DEBUG_INFO(...)                                                                \
    BALLISTICS_DEBUG_CALL(BALLISTICS_DEBUG_LEVEL_INFO, "core", __VA_ARGS__)
#if defined(BALLISTICS_RELEASE_LOGGING) && BALLISTICS_RELEASE_LOGGING
#define BALLISTICS_DEBUG_DEBUG(...) ((void)0)
#define BALLISTICS_DEBUG_TRACE(...) ((void)0)
#define BALLISTICS_PHYSICS_DEBUG(...) ((void)0)
#define BALLISTICS_EQUATION_DEBUG(...) ((void)0)
#define BALLISTICS_INTEGRATOR_DEBUG(...) ((void)0)
#define BALLISTICS_SIMULATION_DEBUG(...) ((void)0)
#define BALLISTICS_PORT_DEBUG(...) ((void)0)
#define BALLISTICS_OUTPUT_DEBUG(...) ((void)0)
#else
#define BALLISTICS_DEBUG_DEBUG(...)                                                               \
    BALLISTICS_DEBUG_CALL(BALLISTICS_DEBUG_LEVEL_DEBUG, "core", __VA_ARGS__)
#define BALLISTICS_DEBUG_TRACE(...)                                                               \
    BALLISTICS_DEBUG_CALL(BALLISTICS_DEBUG_LEVEL_TRACE, "core", __VA_ARGS__)
#define BALLISTICS_PHYSICS_DEBUG(...)                                                             \
    BALLISTICS_DEBUG_CALL(BALLISTICS_DEBUG_LEVEL_DEBUG, "physics", __VA_ARGS__)
#define BALLISTICS_EQUATION_DEBUG(...)                                                            \
    BALLISTICS_DEBUG_CALL(BALLISTICS_DEBUG_LEVEL_DEBUG, "equation", __VA_ARGS__)
#define BALLISTICS_INTEGRATOR_DEBUG(...)                                                          \
    BALLISTICS_DEBUG_CALL(BALLISTICS_DEBUG_LEVEL_DEBUG, "integrator", __VA_ARGS__)
#define BALLISTICS_SIMULATION_DEBUG(...)                                                          \
    BALLISTICS_DEBUG_CALL(BALLISTICS_DEBUG_LEVEL_DEBUG, "simulation", __VA_ARGS__)
#define BALLISTICS_PORT_DEBUG(...)                                                                \
    BALLISTICS_DEBUG_CALL(BALLISTICS_DEBUG_LEVEL_DEBUG, "port", __VA_ARGS__)
#define BALLISTICS_OUTPUT_DEBUG(...)                                                              \
    BALLISTICS_DEBUG_CALL(BALLISTICS_DEBUG_LEVEL_DEBUG, "output", __VA_ARGS__)
#endif
#else
#define BALLISTICS_DEBUG_ERROR(...) ((void)0)
#define BALLISTICS_DEBUG_WARNING(...) ((void)0)
#define BALLISTICS_DEBUG_INFO(...) ((void)0)
#define BALLISTICS_DEBUG_DEBUG(...) ((void)0)
#define BALLISTICS_DEBUG_TRACE(...) ((void)0)
#define BALLISTICS_PHYSICS_DEBUG(...) ((void)0)
#define BALLISTICS_EQUATION_DEBUG(...) ((void)0)
#define BALLISTICS_INTEGRATOR_DEBUG(...) ((void)0)
#define BALLISTICS_SIMULATION_DEBUG(...) ((void)0)
#define BALLISTICS_PORT_DEBUG(...) ((void)0)
#define BALLISTICS_OUTPUT_DEBUG(...) ((void)0)
#endif

#endif
