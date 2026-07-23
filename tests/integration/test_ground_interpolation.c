#include "test_scenario.h"
#include "unity.h"

#include <math.h>

static double impact_time_error(double time_step_s)
{
    BallisticsTestScenarioConfig config = ballistics_test_scenario_defaults();
    BallisticsTestScenario scenario;
    BallisticsProjectileState impact_state;
    BallisticsStopReason reason;
    double impact_time_s;
    const double gravity = 9.80665;
    const double exact_time_s =
        (2.0 + sqrt(2.0 * 2.0 + 2.0 * gravity * 1.0)) / gravity;

    config.time_step_s = time_step_s;
    config.maximum_time_s = 2.0;
    config.ground_height_m = 0.0;
    config.initial_state = (BallisticsProjectileState){{0.0, 0.0, 1.0}, {20.0, 0.0, 2.0}};
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_OK, ballistics_test_scenario_run(&config, &scenario));
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_OK,
                      ballistics_simulation_result_stop_reason(scenario.result, &reason));
    TEST_ASSERT_EQUAL(BALLISTICS_STOP_REASON_GROUND_INTERSECTION, reason);
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_OK,
                      ballistics_simulation_result_final_time(scenario.result, &impact_time_s));
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_OK,
                      ballistics_simulation_result_final_state(scenario.result, &impact_state));
    TEST_ASSERT_DOUBLE_WITHIN(1e-14, 0.0, impact_state.position_m.z);
    {
        const double error = fabs(impact_time_s - exact_time_s);
        ballistics_test_scenario_destroy(&scenario);
        return error;
    }
}

void test_ground_termination_and_interpolation(void)
{
    const double coarse_error = impact_time_error(1.0e-3);
    const double fine_error = impact_time_error(5.0e-4);
    TEST_ASSERT_LESS_OR_EQUAL_DOUBLE(1.0e-5, coarse_error);
    TEST_ASSERT_LESS_THAN_DOUBLE(coarse_error, fine_error);
}
