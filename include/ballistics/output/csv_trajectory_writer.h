#ifndef BALLISTICS_OUTPUT_CSV_TRAJECTORY_WRITER_H
#define BALLISTICS_OUTPUT_CSV_TRAJECTORY_WRITER_H

#include "ballistics/interfaces/byte_sink.h"
#include "ballistics/interfaces/trajectory_writer_interface.h"
#include <stddef.h>

#define BALLISTICS_CSV_WRITER_ID "csv-writer.v1"
#define BALLISTICS_CSV_FLOAT_FORMAT "%.9e"

typedef struct
{
    BallisticsByteSink sink;
} BallisticsCsvWriterConfig;

BALLISTICS_API BallisticsStatus ballistics_csv_trajectory_writer_create(
    const BallisticsCsvWriterConfig *config,
    BallisticsTrajectoryWriter **out_writer);
BALLISTICS_API BallisticsStatus ballistics_csv_trajectory_writer_factory(
    const void *config, size_t config_size, BallisticsTrajectoryWriter **out_writer);

#endif
