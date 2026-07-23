#include "test_memory_sink.h"
#include "test_scenario.h"
#include "unity.h"

#include <string.h>

static const char expected_header[] =
    "time_s,position_x_m,position_y_m,position_z_m,velocity_x_mps,velocity_y_mps,"
    "velocity_z_mps,speed_mps,acceleration_x_mps2,acceleration_y_mps2,"
    "acceleration_z_mps2\n";

void test_csv_header_and_row_count(void)
{
    BallisticsTestScenarioConfig config = ballistics_test_scenario_defaults();
    BallisticsTestScenario scenario;
    BallisticsTestMemorySink sink;
    BallisticsCsvWriterConfig writer_config;
    BallisticsTrajectoryWriter *writer = NULL;
    size_t sample_count;
    size_t newline_count = 0U;
    size_t index;

    config.maximum_time_s = 0.02;
    config.time_step_s = 0.01;
    ballistics_test_memory_sink_init(&sink);
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_OK, ballistics_test_scenario_run(&config, &scenario));
    writer_config.sink = ballistics_test_memory_sink_interface(&sink);
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_OK,
                      ballistics_csv_trajectory_writer_create(&writer_config, &writer));
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_OK,
                      ballistics_trajectory_writer_write_result(writer, scenario.result));
    TEST_ASSERT_GREATER_OR_EQUAL_UINT64(sizeof(expected_header) - 1U, sink.size);
    TEST_ASSERT_EQUAL_MEMORY(expected_header, sink.data, sizeof(expected_header) - 1U);
    for (index = 0U; index < sink.size; ++index)
    {
        if (sink.data[index] == (unsigned char)'\n')
        {
            ++newline_count;
        }
    }
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_OK,
                      ballistics_simulation_result_sample_count(scenario.result, &sample_count));
    TEST_ASSERT_EQUAL_UINT64(sample_count + 1U, newline_count);
    ballistics_trajectory_writer_destroy(writer);
    ballistics_test_memory_sink_destroy(&sink);
    ballistics_test_scenario_destroy(&scenario);
}
