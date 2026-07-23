#include "ballistics/integrators/rk4_integrator.h"
#include "unity.h"

#include <math.h>

static BallisticsStatus exponential_derivative(double time_s,
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

static BallisticsStatus nonfinite_derivative(double time_s,
                                             const double *state,
                                             size_t state_count,
                                             double *out_derivative,
                                             void *context)
{
    (void)time_s;
    (void)state;
    (void)state_count;
    (void)context;
    out_derivative[0] = NAN;
    return BALLISTICS_STATUS_OK;
}

void test_rk4_integrates_exponential_derivative(void)
{
    const BallisticsRk4IntegratorConfig config = {1U};
    BallisticsIntegrator *integrator = NULL;
    const double current[1] = {2.0};
    double output[1] = {0.0};
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_OK, ballistics_rk4_integrator_create(&config, &integrator));
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_OK,
                      ballistics_integrator_step(
                          integrator, current, 1U, 0.0, 0.1, exponential_derivative, NULL, output));
    TEST_ASSERT_DOUBLE_WITHIN(2e-7, 2.0 * exp(0.1), output[0]);
    ballistics_integrator_destroy(integrator);
}

void test_rk4_rejects_nonfinite_derivative(void)
{
    const BallisticsRk4IntegratorConfig config = {1U};
    BallisticsIntegrator *integrator = NULL;
    const double current[1] = {2.0};
    double output[1] = {0.0};
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_OK, ballistics_rk4_integrator_create(&config, &integrator));
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_NUMERICAL_ERROR,
                      ballistics_integrator_step(
                          integrator, current, 1U, 0.0, 0.1, nonfinite_derivative, NULL, output));
    ballistics_integrator_destroy(integrator);
}
