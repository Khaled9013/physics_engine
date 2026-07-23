#include "test_scenario.h"
#include "unity.h"

#include <math.h>

static double nonlinear_position_error(const char *integrator_id, double time_step_s)
{
    BallisticsTestScenarioConfig config = ballistics_test_scenario_defaults();
    BallisticsTestScenario scenario;
    const BallisticsTrajectorySample *final_sample = NULL;
    const double initial_velocity_mps = 50.0;
    double k;
    double exact_position_m;

    config.integrator_id = integrator_id;
    config.time_step_s = time_step_s;
    config.gravity_mps2 = (BallisticsVector3){0.0, 0.0, 0.0};
    config.density_kgpm3 = 1.2;
    config.drag_coefficient = 0.5;
    config.projectile = (BallisticsProjectile){1.0, 0.1, 0.01};
    config.initial_state =
        (BallisticsProjectileState){{0.0, 0.0, 0.0}, {initial_velocity_mps, 0.0, 0.0}};
    k = 0.5 * config.density_kgpm3 * config.drag_coefficient *
        config.projectile.reference_area_m2 / config.projectile.mass_kg;
    exact_position_m = log(1.0 + k * initial_velocity_mps * config.maximum_time_s) / k;
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_OK, ballistics_test_scenario_run(&config, &scenario));
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_OK,
                      ballistics_test_scenario_final_sample(&scenario, &final_sample));
    {
        const double error = fabs(final_sample->state.position_m.x - exact_position_m);
        ballistics_test_scenario_destroy(&scenario);
        return error;
    }
}

void test_quadratic_drag_convergence_order(void)
{
    /* dt=0.2 keeps RK4 truncation error well above 1e-10 for a meaningful ratio. */
    const double rk4_coarse = nonlinear_position_error(BALLISTICS_RK4_INTEGRATOR_ID, 0.2);
    const double rk4_fine = nonlinear_position_error(BALLISTICS_RK4_INTEGRATOR_ID, 0.1);
    const double euler_coarse = nonlinear_position_error(BALLISTICS_EULER_INTEGRATOR_ID, 0.2);
    const double euler_fine = nonlinear_position_error(BALLISTICS_EULER_INTEGRATOR_ID, 0.1);
    const double rk4_order = log(rk4_coarse / rk4_fine) / log(2.0);
    const double euler_order = log(euler_coarse / euler_fine) / log(2.0);

    TEST_ASSERT_GREATER_OR_EQUAL_DOUBLE(1e-10, rk4_coarse);
    TEST_ASSERT_GREATER_OR_EQUAL_DOUBLE(3.5, rk4_order);
    TEST_ASSERT_GREATER_OR_EQUAL_DOUBLE(0.8, euler_order);
    TEST_ASSERT_LESS_OR_EQUAL_DOUBLE(1.2, euler_order);
}
