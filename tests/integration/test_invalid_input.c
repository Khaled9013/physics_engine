#include "ballistics/ballistics.h"
#include "unity.h"

#include <math.h>
#include <stdint.h>

void test_invalid_inputs_return_errors(void)
{
    BallisticsProjectile projectile = {0.0, 0.01, 0.01};
    BallisticsSimulationConfig config = {0.0, 1.0, 10.0, 0.0, "rk4.v1"};
    BallisticsVector3 vector = {INFINITY, 0.0, 0.0};
    BallisticsEquationRegistry *registry = (BallisticsEquationRegistry *)(uintptr_t)1U;

    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_INVALID_ARGUMENT,
                      ballistics_projectile_validate(&projectile));
    projectile.mass_kg = NAN;
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_INVALID_ARGUMENT,
                      ballistics_projectile_validate(&projectile));
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_INVALID_ARGUMENT,
                      ballistics_simulation_config_validate(&config, NULL));
    TEST_ASSERT_FALSE(ballistics_vector3_is_finite(&vector));
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_INVALID_ARGUMENT,
                      ballistics_equation_registry_create(0U, &registry));
    TEST_ASSERT_NULL(registry);
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_INVALID_ARGUMENT,
                      ballistics_simulation_result_create(1U, NULL));
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_INVALID_ARGUMENT,
                      ballistics_simulation_run(NULL, NULL));
}
