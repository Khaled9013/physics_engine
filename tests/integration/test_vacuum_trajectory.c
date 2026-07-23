#include "test_scenario.h"
#include "unity.h"

#include <math.h>

static double vacuum_position_error(const char *integrator_id, double time_step_s)
{
    BallisticsTestScenarioConfig config = ballistics_test_scenario_defaults();
    BallisticsTestScenario scenario;
    const BallisticsTrajectorySample *final_sample = NULL;
    const double expected_z = 10.0 + 30.0 * config.maximum_time_s -
                              0.5 * 9.80665 * config.maximum_time_s * config.maximum_time_s;

    config.integrator_id = integrator_id;
    config.time_step_s = time_step_s;
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_OK, ballistics_test_scenario_run(&config, &scenario));
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_OK,
                      ballistics_test_scenario_final_sample(&scenario, &final_sample));
    {
        const double error = fabs(final_sample->state.position_m.z - expected_z);
        ballistics_test_scenario_destroy(&scenario);
        return error;
    }
}

void test_vacuum_trajectory_exactness_and_euler_refinement(void)
{
    const double rk4_error = vacuum_position_error(BALLISTICS_RK4_INTEGRATOR_ID, 0.01);
    const double euler_coarse_error = vacuum_position_error(BALLISTICS_EULER_INTEGRATOR_ID, 0.2);
    const double euler_fine_error = vacuum_position_error(BALLISTICS_EULER_INTEGRATOR_ID, 0.1);
    TEST_ASSERT_LESS_THAN_DOUBLE(1e-9, rk4_error);
    TEST_ASSERT_TRUE(isfinite(euler_coarse_error));
    TEST_ASSERT_LESS_THAN_DOUBLE(euler_coarse_error, euler_fine_error);
}

void test_zero_gravity_motion(void)
{
    BallisticsTestScenarioConfig config = ballistics_test_scenario_defaults();
    BallisticsTestScenario scenario;
    const BallisticsTrajectorySample *final_sample = NULL;
    config.gravity_mps2 = (BallisticsVector3){0.0, 0.0, 0.0};
    config.initial_state = (BallisticsProjectileState){{1.0, 2.0, 3.0}, {4.0, -5.0, 6.0}};
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_OK, ballistics_test_scenario_run(&config, &scenario));
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_OK,
                      ballistics_test_scenario_final_sample(&scenario, &final_sample));
    TEST_ASSERT_DOUBLE_WITHIN(1e-12, 5.0, final_sample->state.position_m.x);
    TEST_ASSERT_DOUBLE_WITHIN(1e-12, -3.0, final_sample->state.position_m.y);
    TEST_ASSERT_DOUBLE_WITHIN(1e-12, 9.0, final_sample->state.position_m.z);
    TEST_ASSERT_DOUBLE_WITHIN(1e-12, 4.0, final_sample->state.velocity_mps.x);
    TEST_ASSERT_DOUBLE_WITHIN(1e-12, -5.0, final_sample->state.velocity_mps.y);
    TEST_ASSERT_DOUBLE_WITHIN(1e-12, 6.0, final_sample->state.velocity_mps.z);
    ballistics_test_scenario_destroy(&scenario);
}
