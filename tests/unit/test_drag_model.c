#include "ballistics/models/basic_drag_model.h"
#include "unity.h"

void test_drag_model_force(void)
{
    const BallisticsBasicDragConfig config = {0.5};
    const BallisticsProjectile projectile = {1.0, 0.1, 0.01};
    const BallisticsProjectileState state = {{0.0, 0.0, 0.0}, {10.0, 0.0, 0.0}};
    const BallisticsEnvironmentState environment = {1.2, {0.0, 0.0, 0.0}};
    BallisticsForceModel *model = NULL;
    BallisticsVector3 force;
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_OK, ballistics_basic_drag_model_create(&config, &model));
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_OK,
                      ballistics_force_model_calculate_force(
                          model, &projectile, &state, &environment, &force));
    TEST_ASSERT_DOUBLE_WITHIN(1e-15, -0.3, force.x);
    ballistics_force_model_destroy(model);
}
