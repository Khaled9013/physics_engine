#include "ballistics/output/csv_trajectory_writer.h"

#include "ballistics/math/vector3.h"
#include "ballistics/port/ballistics_port.h"

#include <stdio.h>
#include <stddef.h>
#include <string.h>

#define BALLISTICS_CSV_LINE_CAPACITY 512U

static const char csv_header[] =
    "time_s,position_x_m,position_y_m,position_z_m,velocity_x_mps,velocity_y_mps,"
    "velocity_z_mps,speed_mps,acceleration_x_mps2,acceleration_y_mps2,"
    "acceleration_z_mps2\n";

typedef struct
{
    BallisticsByteSink sink;
} BallisticsCsvWriterContext;

static BallisticsStatus sink_write(const BallisticsByteSink *sink, const void *data, size_t size)
{
    if (sink == NULL || sink->write == NULL || (data == NULL && size != 0U))
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    return sink->write(sink->context, data, size);
}

static BallisticsStatus csv_write_result(BallisticsTrajectoryWriter *self,
                                         const BallisticsSimulationResult *result)
{
    BallisticsCsvWriterContext *context;
    size_t sample_count;
    size_t index;
    BallisticsStatus status;

    if (self == NULL || self->context == NULL || result == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    context = self->context;
    status = sink_write(&context->sink, csv_header, sizeof(csv_header) - 1U);
    if (status != BALLISTICS_STATUS_OK)
    {
        return status;
    }
    status = ballistics_simulation_result_sample_count(result, &sample_count);
    if (status != BALLISTICS_STATUS_OK)
    {
        return status;
    }
    for (index = 0U; index < sample_count; ++index)
    {
        const BallisticsTrajectorySample *sample;
        char line[BALLISTICS_CSV_LINE_CAPACITY];
        double speed_mps;
        int length;

        status = ballistics_simulation_result_sample_at(result, index, &sample);
        if (status != BALLISTICS_STATUS_OK)
        {
            return status;
        }
        status = ballistics_vector3_magnitude(&sample->state.velocity_mps, &speed_mps);
        if (status != BALLISTICS_STATUS_OK)
        {
            return status;
        }
        length = snprintf(line,
                          sizeof(line),
                          BALLISTICS_CSV_FLOAT_FORMAT "," BALLISTICS_CSV_FLOAT_FORMAT ","
                          BALLISTICS_CSV_FLOAT_FORMAT "," BALLISTICS_CSV_FLOAT_FORMAT ","
                          BALLISTICS_CSV_FLOAT_FORMAT "," BALLISTICS_CSV_FLOAT_FORMAT ","
                          BALLISTICS_CSV_FLOAT_FORMAT "," BALLISTICS_CSV_FLOAT_FORMAT ","
                          BALLISTICS_CSV_FLOAT_FORMAT "," BALLISTICS_CSV_FLOAT_FORMAT ","
                          BALLISTICS_CSV_FLOAT_FORMAT "\n",
                          sample->time_s,
                          sample->state.position_m.x,
                          sample->state.position_m.y,
                          sample->state.position_m.z,
                          sample->state.velocity_mps.x,
                          sample->state.velocity_mps.y,
                          sample->state.velocity_mps.z,
                          speed_mps,
                          sample->acceleration_mps2.x,
                          sample->acceleration_mps2.y,
                          sample->acceleration_mps2.z);
        if (length < 0 || (size_t)length >= sizeof(line))
        {
            return BALLISTICS_STATUS_INTERNAL_ERROR;
        }
        status = sink_write(&context->sink, line, (size_t)length);
        if (status != BALLISTICS_STATUS_OK)
        {
            return status;
        }
    }
    if (context->sink.flush != NULL)
    {
        return context->sink.flush(context->sink.context);
    }
    return BALLISTICS_STATUS_OK;
}

static void csv_destroy(BallisticsTrajectoryWriter *writer)
{
    if (writer != NULL)
    {
        (void)ballistics_port_deallocate(writer->context);
        (void)ballistics_port_deallocate(writer);
    }
}

static const BallisticsTrajectoryWriterVTable csv_vtable = {csv_write_result, csv_destroy};

BallisticsStatus ballistics_csv_trajectory_writer_create(
    const BallisticsCsvWriterConfig *config,
    BallisticsTrajectoryWriter **out_writer)
{
    BallisticsTrajectoryWriter *writer = NULL;
    BallisticsCsvWriterContext *context = NULL;
    void *memory = NULL;
    BallisticsStatus status;

    if (out_writer == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    *out_writer = NULL;
    if (config == NULL || config->sink.write == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    status = ballistics_port_allocate(sizeof(*writer), &memory);
    if (status != BALLISTICS_STATUS_OK)
    {
        return status;
    }
    writer = memory;
    status = ballistics_port_allocate(sizeof(*context), &memory);
    if (status != BALLISTICS_STATUS_OK)
    {
        (void)ballistics_port_deallocate(writer);
        return status;
    }
    context = memory;
    context->sink = config->sink;
    writer->vtable = &csv_vtable;
    writer->context = context;
    writer->name = BALLISTICS_CSV_WRITER_ID;
    *out_writer = writer;
    return BALLISTICS_STATUS_OK;
}

BallisticsStatus ballistics_csv_trajectory_writer_factory(
    const void *config, size_t config_size, BallisticsTrajectoryWriter **out_writer)
{
    if (config == NULL || config_size != sizeof(BallisticsCsvWriterConfig))
    {
        if (out_writer != NULL)
        {
            *out_writer = NULL;
        }
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    return ballistics_csv_trajectory_writer_create(config, out_writer);
}
