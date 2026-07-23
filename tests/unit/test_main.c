#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

void test_vector_addition(void);
void test_vector_subtraction(void);
void test_vector_scaling(void);
void test_vector_magnitude(void);
void test_vector_normalization(void);
void test_vector_zero_normalization(void);
void test_vector_finite_validation(void);
void test_air_relative_velocity(void);
void test_drag_equation_direction_and_magnitude(void);
void test_acceleration_equation(void);
void test_equation_generic_size_validation(void);
void test_gravity_force(void);
void test_drag_model_force(void);
void test_registry_registration_duplicate_and_missing(void);
void test_registry_invalid_create_clears_output(void);
void test_factory_config_size_validation(void);
void test_configuration_validation(void);
void test_status_string_conversion(void);
void test_euler_integrates_linear_derivative(void);
void test_euler_rejects_capacity(void);
void test_rk4_integrates_exponential_derivative(void);
void test_rk4_rejects_nonfinite_derivative(void);

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_vector_addition);
    RUN_TEST(test_vector_subtraction);
    RUN_TEST(test_vector_scaling);
    RUN_TEST(test_vector_magnitude);
    RUN_TEST(test_vector_normalization);
    RUN_TEST(test_vector_zero_normalization);
    RUN_TEST(test_vector_finite_validation);
    RUN_TEST(test_air_relative_velocity);
    RUN_TEST(test_drag_equation_direction_and_magnitude);
    RUN_TEST(test_acceleration_equation);
    RUN_TEST(test_equation_generic_size_validation);
    RUN_TEST(test_gravity_force);
    RUN_TEST(test_drag_model_force);
    RUN_TEST(test_registry_registration_duplicate_and_missing);
    RUN_TEST(test_registry_invalid_create_clears_output);
    RUN_TEST(test_factory_config_size_validation);
    RUN_TEST(test_configuration_validation);
    RUN_TEST(test_status_string_conversion);
    RUN_TEST(test_euler_integrates_linear_derivative);
    RUN_TEST(test_euler_rejects_capacity);
    RUN_TEST(test_rk4_integrates_exponential_derivative);
    RUN_TEST(test_rk4_rejects_nonfinite_derivative);
    return UNITY_END();
}
