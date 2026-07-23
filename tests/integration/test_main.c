#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

void test_vacuum_trajectory_exactness_and_euler_refinement(void);
void test_zero_gravity_motion(void);
void test_drag_deceleration_monotonic(void);
void test_integrator_comparison(void);
void test_quadratic_drag_convergence_order(void);
void test_crosswind_symmetry(void);
void test_ground_termination_and_interpolation(void);
void test_deterministic_csv_output(void);
void test_csv_header_and_row_count(void);
void test_invalid_inputs_return_errors(void);

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_vacuum_trajectory_exactness_and_euler_refinement);
    RUN_TEST(test_zero_gravity_motion);
    RUN_TEST(test_drag_deceleration_monotonic);
    RUN_TEST(test_integrator_comparison);
    RUN_TEST(test_quadratic_drag_convergence_order);
    RUN_TEST(test_crosswind_symmetry);
    RUN_TEST(test_ground_termination_and_interpolation);
    RUN_TEST(test_deterministic_csv_output);
    RUN_TEST(test_csv_header_and_row_count);
    RUN_TEST(test_invalid_inputs_return_errors);
    return UNITY_END();
}
