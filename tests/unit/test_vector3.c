#include "ballistics/math/vector3.h"
#include "unity.h"

#include <float.h>
#include <math.h>

void test_vector_addition(void)
{
    const BallisticsVector3 a = {1.0, -2.0, 3.0};
    const BallisticsVector3 b = {4.0, 5.0, -6.0};
    BallisticsVector3 result;
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_OK, ballistics_vector3_add(&a, &b, &result));
    TEST_ASSERT_DOUBLE_WITHIN(1e-15, 5.0, result.x);
    TEST_ASSERT_DOUBLE_WITHIN(1e-15, 3.0, result.y);
    TEST_ASSERT_DOUBLE_WITHIN(1e-15, -3.0, result.z);
}

void test_vector_subtraction(void)
{
    const BallisticsVector3 a = {5.0, 3.0, -3.0};
    const BallisticsVector3 b = {4.0, 5.0, -6.0};
    BallisticsVector3 result;
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_OK, ballistics_vector3_subtract(&a, &b, &result));
    TEST_ASSERT_EQUAL_DOUBLE(1.0, result.x);
    TEST_ASSERT_EQUAL_DOUBLE(-2.0, result.y);
    TEST_ASSERT_EQUAL_DOUBLE(3.0, result.z);
}

void test_vector_scaling(void)
{
    const BallisticsVector3 value = {1.0, -2.0, 3.0};
    BallisticsVector3 result;
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_OK, ballistics_vector3_scale(&value, 2.5, &result));
    TEST_ASSERT_EQUAL_DOUBLE(2.5, result.x);
    TEST_ASSERT_EQUAL_DOUBLE(-5.0, result.y);
    TEST_ASSERT_EQUAL_DOUBLE(7.5, result.z);
}

void test_vector_magnitude(void)
{
    const BallisticsVector3 value = {3.0, 4.0, 12.0};
    double magnitude = 0.0;
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_OK, ballistics_vector3_magnitude(&value, &magnitude));
    TEST_ASSERT_DOUBLE_WITHIN(1e-14, 13.0, magnitude);
}

void test_vector_normalization(void)
{
    const BallisticsVector3 extreme = {DBL_MAX, DBL_MAX, 0.0};
    BallisticsVector3 result;
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_OK, ballistics_vector3_normalize(&extreme, &result));
    TEST_ASSERT_DOUBLE_WITHIN(1e-15, 0.7071067811865475, result.x);
    TEST_ASSERT_DOUBLE_WITHIN(1e-15, result.x, result.y);
}

void test_vector_zero_normalization(void)
{
    const BallisticsVector3 zero = {0.0, 0.0, 0.0};
    BallisticsVector3 result = {1.0, 1.0, 1.0};
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_INVALID_ARGUMENT,
                      ballistics_vector3_normalize(&zero, &result));
}

void test_vector_finite_validation(void)
{
    const BallisticsVector3 finite = {1.0, 2.0, 3.0};
    const BallisticsVector3 invalid = {NAN, 2.0, 3.0};
    TEST_ASSERT_TRUE(ballistics_vector3_is_finite(&finite));
    TEST_ASSERT_FALSE(ballistics_vector3_is_finite(&invalid));
    TEST_ASSERT_FALSE(ballistics_vector3_is_finite(NULL));
}
