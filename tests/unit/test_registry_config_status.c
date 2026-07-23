#include "ballistics/equations/air_relative_velocity_equation.h"
#include "ballistics/integrators/euler_integrator.h"
#include "ballistics/registry/builtin_registry.h"
#include "ballistics/simulation_config.h"
#include "ballistics/status.h"
#include "unity.h"

#include <stdint.h>

void test_registry_registration_duplicate_and_missing(void)
{
    BallisticsEquationRegistry *registry = NULL;
    BallisticsEquationFactory factory = NULL;
    const char *identifier = NULL;
    size_t count = 0U;
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_OK, ballistics_equation_registry_create(2U, &registry));
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_OK,
                      ballistics_equation_registry_register(
                          registry, "test-equation.v1", ballistics_air_relative_velocity_equation_factory));
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_DUPLICATE,
                      ballistics_equation_registry_register(
                          registry, "test-equation.v1", ballistics_air_relative_velocity_equation_factory));
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_NOT_FOUND,
                      ballistics_equation_registry_find(registry, "missing.v1", &factory));
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_OK, ballistics_equation_registry_count(registry, &count));
    TEST_ASSERT_EQUAL_UINT64(1U, count);
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_OK,
                      ballistics_equation_registry_entry_identifier(registry, 0U, &identifier));
    TEST_ASSERT_EQUAL_STRING("test-equation.v1", identifier);
    ballistics_equation_registry_destroy(registry);
}

void test_registry_invalid_create_clears_output(void)
{
    BallisticsEquationRegistry *registry = (BallisticsEquationRegistry *)(uintptr_t)1U;
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_INVALID_ARGUMENT,
                      ballistics_equation_registry_create(0U, &registry));
    TEST_ASSERT_NULL(registry);
}

void test_factory_config_size_validation(void)
{
    BallisticsEulerIntegratorConfig config = {6U};
    BallisticsIntegrator *integrator = (BallisticsIntegrator *)(uintptr_t)1U;
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_INVALID_ARGUMENT,
                      ballistics_euler_integrator_factory(
                          &config, sizeof(config) - 1U, &integrator));
    TEST_ASSERT_NULL(integrator);
}

void test_configuration_validation(void)
{
    BallisticsIntegratorRegistry *registry = NULL;
    BallisticsSimulationConfig config = {0.01, 1.0, 100.0, 0.0, "rk4.v1"};
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_OK, ballistics_integrator_registry_create(4U, &registry));
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_OK, ballistics_register_builtin_integrators(registry));
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_OK,
                      ballistics_simulation_config_validate(&config, registry));
    config.time_step_seconds = 0.0;
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_INVALID_ARGUMENT,
                      ballistics_simulation_config_validate(&config, registry));
    config.time_step_seconds = 0.01;
    config.integrator_id = "unknown.v1";
    TEST_ASSERT_EQUAL(BALLISTICS_STATUS_NOT_FOUND,
                      ballistics_simulation_config_validate(&config, registry));
    ballistics_integrator_registry_destroy(registry);
}

void test_status_string_conversion(void)
{
    TEST_ASSERT_EQUAL_STRING("ok", ballistics_status_to_string(BALLISTICS_STATUS_OK));
    TEST_ASSERT_EQUAL_STRING("numerical error",
                             ballistics_status_to_string(BALLISTICS_STATUS_NUMERICAL_ERROR));
    TEST_ASSERT_EQUAL_STRING("unknown status", ballistics_status_to_string((BallisticsStatus)999));
}
