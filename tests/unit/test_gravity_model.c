#include "ballistics/models/constant_gravity_model.h"
#include "unity.h"

void test_gravity_force(void)
{
    const BallisticsConstantGravityConfig config = {{0.0, 0.0, -9.80665}};
    const BallisticsProjectile projectile = {2.0, 0.01, 0.001};
    const BallisticsProjectileState state = {{0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}};
    const BallisticsEnvironmentState environment = {0.0, {0.0, 0.0, 0.0}};
    BallisticsForceModel *model = NULL;
    BallisticsVector3 force;
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_OK,
                      ballistics_constant_gravity_model_create(&config, &model));
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_OK,
                      ballistics_force_model_calculate_force(
                          model, &projectile, &state, &environment, &force));
    TEST_ASSERT_DOUBLE_WITHIN(1e-14, -19.6133, force.z);
    ballistics_force_model_destroy(model);
}
