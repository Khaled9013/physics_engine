#include "test_scenario.h"
#include "unity.h"

#include <math.h>

static double drag_position_error(const char *integrator_id, double time_step_s)
{
    BallisticsTestScenarioConfig config = ballistics_test_scenario_defaults();
    BallisticsTestScenario scenario;
    const BallisticsTrajectorySample *final_sample = NULL;
    double k;
    double exact_x;

    config.integrator_id = integrator_id;
    config.time_step_s = time_step_s;
    config.gravity_mps2 = (BallisticsVector3){0.0, 0.0, 0.0};
    config.density_kgpm3 = 1.2;
    config.drag_coefficient = 0.5;
    config.projectile = (BallisticsProjectile){1.0, 0.1, 0.01};
    config.initial_state = (BallisticsProjectileState){{0.0, 0.0, 0.0}, {50.0, 0.0, 0.0}};
    k = 0.5 * config.density_kgpm3 * config.drag_coefficient *
        config.projectile.reference_area_m2 / config.projectile.mass_kg;
    exact_x = log(1.0 + k * 50.0 * config.maximum_time_s) / k;
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_OK, ballistics_test_scenario_run(&config, &scenario));
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_OK,
                      ballistics_test_scenario_final_sample(&scenario, &final_sample));
    {
        const double error = fabs(final_sample->state.position_m.x - exact_x);
        ballistics_test_scenario_destroy(&scenario);
        return error;
    }
}

void test_drag_deceleration_monotonic(void)
{
    BallisticsTestScenarioConfig config = ballistics_test_scenario_defaults();
    BallisticsTestScenario scenario;
    const BallisticsTrajectorySample *sample = NULL;
    size_t count;
    size_t index;
    double previous_speed = 1.0e300;

    config.gravity_mps2 = (BallisticsVector3){0.0, 0.0, 0.0};
    config.density_kgpm3 = 1.2;
    config.drag_coefficient = 0.5;
    config.initial_state = (BallisticsProjectileState){{0.0, 0.0, 0.0}, {50.0, 0.0, 0.0}};
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_OK, ballistics_test_scenario_run(&config, &scenario));
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_OK,
                      ballistics_simulation_result_sample_count(scenario.result, &count));
    for (index = 0U; index < count; ++index)
    {
        double speed;
        TEST_ASSERT_EQUAL(BALLISTICS_STATUS_OK,
                          ballistics_simulation_result_sample_at(scenario.result, index, &sample));
        TEST_ASSERT_EQUAL(BALLISTICS_STATUS_OK,
                          ballistics_vector3_magnitude(&sample->state.velocity_mps, &speed));
        TEST_ASSERT_LESS_OR_EQUAL_DOUBLE(previous_speed + 1e-12, speed);
        previous_speed = speed;
    }
    ballistics_test_scenario_destroy(&scenario);
}

void test_integrator_comparison(void)
{
    const double rk4_error = drag_position_error(BALLISTICS_RK4_INTEGRATOR_ID, 0.2);
    const double euler_error = drag_position_error(BALLISTICS_EULER_INTEGRATOR_ID, 0.2);
    TEST_ASSERT_LESS_THAN_DOUBLE(euler_error, rk4_error);
}
