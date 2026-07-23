#include "ballistics/integrators/euler_integrator.h"
#include "unity.h"

static BallisticsStatus linear_derivative(double time_s,
                                          const double *state,
                                          size_t state_count,
                                          double *out_derivative,
                                          void *context)
{
    size_t index;
    (void)time_s;
    (void)context;
    for (index = 0U; index < state_count; ++index)
    {
        out_derivative[index] = state[index];
    }
    return BALLISTICS_STATUS_OK;
}

void test_euler_integrates_linear_derivative(void)
{
    const BallisticsEulerIntegratorConfig config = {2U};
    BallisticsIntegrator *integrator = NULL;
    const double current[1] = {2.0};
    double output[1] = {0.0};
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_OK, ballistics_euler_integrator_create(&config, &integrator));
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_OK,
                      ballistics_integrator_step(
                          integrator, current, 1U, 0.0, 0.5, linear_derivative, NULL, output));
    TEST_ASSERT_EQUAL_DOUBLE(3.0, output[0]);
    ballistics_integrator_destroy(integrator);
}

void test_euler_rejects_capacity(void)
{
    const BallisticsEulerIntegratorConfig config = {1U};
    BallisticsIntegrator *integrator = NULL;
    const double current[2] = {1.0, 2.0};
    double output[2] = {0.0, 0.0};
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_OK, ballistics_euler_integrator_create(&config, &integrator));
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_CAPACITY_EXCEEDED,
                      ballistics_integrator_step(
                          integrator, current, 2U, 0.0, 0.1, linear_derivative, NULL, output));
    ballistics_integrator_destroy(integrator);
}
