#include "test_memory_sink.h"
#include "test_scenario.h"
#include "unity.h"

#include <string.h>

static void write_scenario_csv(BallisticsTestMemorySink *sink)
{
    BallisticsTestScenarioConfig scenario_config = ballistics_test_scenario_defaults();
    BallisticsTestScenario scenario;
    BallisticsCsvWriterConfig writer_config;
    BallisticsTrajectoryWriter *writer = NULL;

    scenario_config.maximum_time_s = 0.1;
    scenario_config.time_step_s = 0.01;
    ballistics_test_memory_sink_init(sink);
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_OK,
                      ballistics_test_scenario_run(&scenario_config, &scenario));
    writer_config.sink = ballistics_test_memory_sink_interface(sink);
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_OK,
                      ballistics_csv_trajectory_writer_create(&writer_config, &writer));
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_OK,
                      ballistics_trajectory_writer_write_result(writer, scenario.result));
    ballistics_trajectory_writer_destroy(writer);
    ballistics_test_scenario_destroy(&scenario);
}

void test_deterministic_csv_output(void)
{
    BallisticsTestMemorySink first;
    BallisticsTestMemorySink second;
    write_scenario_csv(&first);
    write_scenario_csv(&second);
    TEST_ASSERT_EQUAL_UINT64(first.size, second.size);
    TEST_ASSERT_EQUAL_MEMORY(first.data, second.data, first.size);
    ballistics_test_memory_sink_destroy(&second);
    ballistics_test_memory_sink_destroy(&first);
}
