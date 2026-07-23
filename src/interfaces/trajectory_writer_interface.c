#include "ballistics/interfaces/trajectory_writer_interface.h"

#include <stddef.h>

BallisticsStatus ballistics_trajectory_writer_write_result(
    BallisticsTrajectoryWriter *writer,
    const BallisticsSimulationResult *result)
{
    if (writer == NULL || writer->vtable == NULL || writer->vtable->write_result == NULL ||
        result == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    return writer->vtable->write_result(writer, result);
}

void ballistics_trajectory_writer_destroy(BallisticsTrajectoryWriter *writer)
{
    if (writer != NULL && writer->vtable != NULL && writer->vtable->destroy != NULL)
    {
        writer->vtable->destroy(writer);
    }
}
