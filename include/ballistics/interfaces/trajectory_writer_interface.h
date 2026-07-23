#ifndef BALLISTICS_INTERFACES_TRAJECTORY_WRITER_INTERFACE_H
#define BALLISTICS_INTERFACES_TRAJECTORY_WRITER_INTERFACE_H

#include "ballistics/export.h"
#include "ballistics/simulation_result.h"
#include "ballistics/status.h"

typedef struct BallisticsTrajectoryWriter BallisticsTrajectoryWriter;
typedef struct
{
    BallisticsStatus (*write_result)(BallisticsTrajectoryWriter *self,
                                     const BallisticsSimulationResult *result);
    void (*destroy)(BallisticsTrajectoryWriter *self);
} BallisticsTrajectoryWriterVTable;

struct BallisticsTrajectoryWriter
{
    const BallisticsTrajectoryWriterVTable *vtable;
    void *context;
    const char *name;
};

BALLISTICS_API BallisticsStatus ballistics_trajectory_writer_write_result(
    BallisticsTrajectoryWriter *writer,
    const BallisticsSimulationResult *result);
BALLISTICS_API void ballistics_trajectory_writer_destroy(BallisticsTrajectoryWriter *writer);

#endif
