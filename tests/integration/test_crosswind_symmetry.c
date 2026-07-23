#include "test_scenario.h"
#include "unity.h"

static BallisticsProjectileState run_crosswind(double wind_y_mps)
{
    BallisticsTestScenarioConfig config = ballistics_test_scenario_defaults();
    BallisticsTestScenario scenario;
    const BallisticsTrajectorySample *final_sample = NULL;
    BallisticsProjectileState final_state;

    config.gravity_mps2 = (BallisticsVector3){0.0, 0.0, 0.0};
    config.density_kgpm3 = 1.2;
    config.drag_coefficient = 0.5;
    config.wind_mps = (BallisticsVector3){0.0, wind_y_mps, 0.0};
    config.initial_state = (BallisticsProjectileState){{0.0, 0.0, 0.0}, {50.0, 0.0, 0.0}};
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_OK, ballistics_test_scenario_run(&config, &scenario));
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_OK,
                      ballistics_test_scenario_final_sample(&scenario, &final_sample));
    final_state = final_sample->state;
    ballistics_test_scenario_destroy(&scenario);
    return final_state;
}

void test_crosswind_symmetry(void)
{
    const BallisticsProjectileState positive = run_crosswind(10.0);
    const BallisticsProjectileState negative = run_crosswind(-10.0);
    TEST_ASSERT_DOUBLE_WITHIN(1e-11, positive.position_m.x, negative.position_m.x);
    TEST_ASSERT_DOUBLE_WITHIN(1e-11, positive.position_m.y, -negative.position_m.y);
    TEST_ASSERT_DOUBLE_WITHIN(1e-11, positive.velocity_mps.y, -negative.velocity_mps.y);
}
