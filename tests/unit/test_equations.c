#include "ballistics/equations/acceleration_equation.h"
#include "ballistics/equations/aerodynamic_drag_equation.h"
#include "ballistics/equations/air_relative_velocity_equation.h"
#include "unity.h"

void test_air_relative_velocity(void)
{
    const BallisticsAirRelativeVelocityInput input = {{10.0, 2.0, 1.0}, {3.0, -1.0, 1.0}};
    BallisticsVector3 result;
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_OK,
                      ballistics_air_relative_velocity_evaluate(&input, &result));
    TEST_ASSERT_EQUAL_DOUBLE(7.0, result.x);
    TEST_ASSERT_EQUAL_DOUBLE(3.0, result.y);
    TEST_ASSERT_EQUAL_DOUBLE(0.0, result.z);
}

void test_drag_equation_direction_and_magnitude(void)
{
    const BallisticsAerodynamicDragInput input = {{10.0, 0.0, 0.0}, 1.2, 0.5, 0.01};
    BallisticsVector3 force;
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_OK, ballistics_aerodynamic_drag_evaluate(&input, &force));
    TEST_ASSERT_DOUBLE_WITHIN(1e-15, -0.3, force.x);
    TEST_ASSERT_EQUAL_DOUBLE(0.0, force.y);
    TEST_ASSERT_EQUAL_DOUBLE(0.0, force.z);
}

void test_acceleration_equation(void)
{
    const BallisticsAccelerationInput input = {{4.0, -2.0, 6.0}, 2.0};
    BallisticsVector3 acceleration;
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_OK,
                      ballistics_acceleration_evaluate(&input, &acceleration));
    TEST_ASSERT_EQUAL_DOUBLE(2.0, acceleration.x);
    TEST_ASSERT_EQUAL_DOUBLE(-1.0, acceleration.y);
    TEST_ASSERT_EQUAL_DOUBLE(3.0, acceleration.z);
}

void test_equation_generic_size_validation(void)
{
    BallisticsEquation *equation = NULL;
    BallisticsAirRelativeVelocityInput input = {{1.0, 0.0, 0.0}, {0.0, 0.0, 0.0}};
    BallisticsVector3 output;
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_OK,
                      ballistics_air_relative_velocity_equation_create(&equation));
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_INVALID_ARGUMENT,
                      ballistics_equation_evaluate(
                          equation, &input, sizeof(input) - 1U, &output, sizeof(output)));
    ballistics_equation_destroy(equation);
}
