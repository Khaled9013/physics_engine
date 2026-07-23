#include "cli_options.h"
#include "unity.h"

#include <math.h>

void setUp(void)
{
}

void tearDown(void)
{
}

static int argument_count(char **arguments, size_t size)
{
    (void)arguments;
    return (int)size;
}

static void test_defaults_preserve_phase_one_scenario(void)
{
    char *arguments[] = {"ballistics_cli"};
    BallisticsCliOptions options;

    TEST_ASSERT_EQUAL_INT(BALLISTICS_STATUS_OK,
                          ballistics_cli_options_parse(
                              argument_count(arguments, sizeof(arguments) / sizeof(arguments[0])),
                              arguments,
                              &options));
    TEST_ASSERT_EQUAL_STRING("rk4.v1", options.integrator_id);
    TEST_ASSERT_DOUBLE_WITHIN(1.0e-15, 0.001, options.time_step_s);
    TEST_ASSERT_DOUBLE_WITHIN(1.0e-15, 10.0, options.maximum_time_s);
    TEST_ASSERT_DOUBLE_WITHIN(1.0e-12, 310.0, options.launch_speed_mps);
    TEST_ASSERT_DOUBLE_WITHIN(1.0e-12, 2.0, options.wind_y_mps);
    TEST_ASSERT_DOUBLE_WITHIN(1.0e-12, 9.80665, options.gravity_mps2);
}

static void test_all_gui_scenario_options_parse(void)
{
    char *arguments[] = {"ballistics_cli",
                         "--integrator",
                         "euler.v1",
                         "--time-step",
                         "0.004",
                         "--max-time",
                         "6",
                         "--max-distance",
                         "800",
                         "--mass",
                         "0.02",
                         "--diameter",
                         "0.01",
                         "--reference-area",
                         "0.00008",
                         "--launch-speed",
                         "250",
                         "--elevation-deg",
                         "7.5",
                         "--azimuth-deg",
                         "-2.5",
                         "--initial-height",
                         "1.7",
                         "--drag-coefficient",
                         "0.31",
                         "--air-density",
                         "1.1",
                         "--wind-x",
                         "3",
                         "--wind-y",
                         "-4",
                         "--wind-z",
                         "1",
                         "--gravity",
                         "8.5",
                         "--output",
                         "custom.csv",
                         "--debug-level",
                         "info"};
    BallisticsCliOptions options;

    TEST_ASSERT_EQUAL_INT(BALLISTICS_STATUS_OK,
                          ballistics_cli_options_parse(
                              argument_count(arguments, sizeof(arguments) / sizeof(arguments[0])),
                              arguments,
                              &options));
    TEST_ASSERT_EQUAL_STRING("euler.v1", options.integrator_id);
    TEST_ASSERT_EQUAL_STRING("custom.csv", options.output_path);
    TEST_ASSERT_DOUBLE_WITHIN(1.0e-12, 800.0, options.maximum_distance_m);
    TEST_ASSERT_DOUBLE_WITHIN(1.0e-12, 7.5, options.elevation_deg);
    TEST_ASSERT_DOUBLE_WITHIN(1.0e-12, -2.5, options.azimuth_deg);
    TEST_ASSERT_DOUBLE_WITHIN(1.0e-12, -4.0, options.wind_y_mps);
    TEST_ASSERT_EQUAL_INT(BALLISTICS_DEBUG_LEVEL_INFO, options.debug_level);
}

static void test_invalid_numeric_values_are_rejected(void)
{
    char *negative_mass[] = {"ballistics_cli", "--mass", "-1"};
    char *nan_wind[] = {"ballistics_cli", "--wind-y", "nan"};
    char *bad_angle[] = {"ballistics_cli", "--elevation-deg", "91"};
    BallisticsCliOptions options;

    TEST_ASSERT_EQUAL_INT(
        BALLISTICS_STATUS_INVALID_ARGUMENT,
        ballistics_cli_options_parse(argument_count(
                                         negative_mass,
                                         sizeof(negative_mass) / sizeof(negative_mass[0])),
                                     negative_mass,
                                     &options));
    TEST_ASSERT_EQUAL_INT(
        BALLISTICS_STATUS_INVALID_ARGUMENT,
        ballistics_cli_options_parse(
            argument_count(nan_wind, sizeof(nan_wind) / sizeof(nan_wind[0])),
            nan_wind,
            &options));
    TEST_ASSERT_EQUAL_INT(
        BALLISTICS_STATUS_INVALID_ARGUMENT,
        ballistics_cli_options_parse(
            argument_count(bad_angle, sizeof(bad_angle) / sizeof(bad_angle[0])),
            bad_angle,
            &options));
}

static void test_unknown_and_missing_options_are_rejected(void)
{
    char *unknown[] = {"ballistics_cli", "--unknown", "1"};
    char *missing[] = {"ballistics_cli", "--time-step"};
    BallisticsCliOptions options;

    TEST_ASSERT_EQUAL_INT(
        BALLISTICS_STATUS_INVALID_ARGUMENT,
        ballistics_cli_options_parse(
            argument_count(unknown, sizeof(unknown) / sizeof(unknown[0])), unknown, &options));
    TEST_ASSERT_EQUAL_INT(
        BALLISTICS_STATUS_INVALID_ARGUMENT,
        ballistics_cli_options_parse(
            argument_count(missing, sizeof(missing) / sizeof(missing[0])), missing, &options));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_defaults_preserve_phase_one_scenario);
    RUN_TEST(test_all_gui_scenario_options_parse);
    RUN_TEST(test_invalid_numeric_values_are_rejected);
    RUN_TEST(test_unknown_and_missing_options_are_rejected);
    return UNITY_END();
}
