#include "ballistics/ballistics.h"
#include "cli_options.h"
#include "file_byte_sink.h"

#include <locale.h>
#include <stdio.h>
#include <stdlib.h>

#define BALLISTICS_CLI_REGISTRY_CAPACITY 8U
#define BALLISTICS_CLI_RESULT_CAPACITY 256U
#define BALLISTICS_CLI_FORCE_MODEL_COUNT 2U
#define BALLISTICS_CLI_STOP_CONDITION_COUNT 4U

static const char *stop_reason_name(BallisticsStopReason reason)
{
    switch (reason)
    {
        case BALLISTICS_STOP_REASON_MAXIMUM_TIME:
            return "maximum time";
        case BALLISTICS_STOP_REASON_GROUND_INTERSECTION:
            return "ground intersection";
        case BALLISTICS_STOP_REASON_MAXIMUM_DISTANCE:
            return "maximum distance";
        case BALLISTICS_STOP_REASON_INVALID_STATE:
            return "invalid state";
        default:
            return "none";
    }
}

static void report_error(const char *operation, BallisticsStatus status)
{
    fprintf(stderr, "ballistics_cli: %s: %s\n", operation, ballistics_status_to_string(status));
}

int main(int argc, char **argv)
{
    BallisticsCliOptions options;
    BallisticsEquationRegistry *equation_registry = NULL;
    BallisticsForceModelRegistry *force_registry = NULL;
    BallisticsIntegratorRegistry *integrator_registry = NULL;
    BallisticsWriterRegistry *writer_registry = NULL;
    BallisticsEnvironmentModel *environment = NULL;
    BallisticsForceModel *gravity = NULL;
    BallisticsForceModel *drag = NULL;
    BallisticsForceModel *force_models[BALLISTICS_CLI_FORCE_MODEL_COUNT];
    BallisticsIntegrator *integrator = NULL;
    BallisticsStopCondition *stop_conditions[BALLISTICS_CLI_STOP_CONDITION_COUNT] = {NULL};
    BallisticsDynamicsContext *dynamics = NULL;
    BallisticsSimulation *simulation = NULL;
    BallisticsSimulationResult *result = NULL;
    BallisticsTrajectoryWriter *writer = NULL;
    FILE *output_file = NULL;
    BallisticsStatus status;
    int exit_code = EXIT_FAILURE;

    status = ballistics_cli_options_parse(argc, argv, &options);
    if (status != BALLISTICS_STATUS_OK)
    {
        ballistics_cli_options_print_usage(stderr, argv != NULL ? argv[0] : "ballistics_cli");
        return 2;
    }
    if (options.show_help)
    {
        ballistics_cli_options_print_usage(stdout, argv[0]);
        return EXIT_SUCCESS;
    }
    if (setlocale(LC_NUMERIC, "C") == NULL)
    {
        fputs("ballistics_cli: unable to select C numeric locale\n", stderr);
        return EXIT_FAILURE;
    }
    status = ballistics_debug_set_level(options.debug_level);
    if (status != BALLISTICS_STATUS_OK)
    {
        report_error("set debug level", status);
        return EXIT_FAILURE;
    }

    status = ballistics_equation_registry_create(BALLISTICS_CLI_REGISTRY_CAPACITY,
                                                 &equation_registry);
    if (status != BALLISTICS_STATUS_OK)
    {
        report_error("create equation registry", status);
        goto cleanup;
    }
    status = ballistics_force_model_registry_create(BALLISTICS_CLI_REGISTRY_CAPACITY,
                                                    &force_registry);
    if (status != BALLISTICS_STATUS_OK)
    {
        report_error("create force registry", status);
        goto cleanup;
    }
    status = ballistics_integrator_registry_create(BALLISTICS_CLI_REGISTRY_CAPACITY,
                                                   &integrator_registry);
    if (status != BALLISTICS_STATUS_OK)
    {
        report_error("create integrator registry", status);
        goto cleanup;
    }
    status = ballistics_writer_registry_create(BALLISTICS_CLI_REGISTRY_CAPACITY,
                                               &writer_registry);
    if (status != BALLISTICS_STATUS_OK)
    {
        report_error("create writer registry", status);
        goto cleanup;
    }
    if ((status = ballistics_register_builtin_equations(equation_registry)) !=
            BALLISTICS_STATUS_OK ||
        (status = ballistics_register_builtin_force_models(force_registry)) !=
            BALLISTICS_STATUS_OK ||
        (status = ballistics_register_builtin_integrators(integrator_registry)) !=
            BALLISTICS_STATUS_OK ||
        (status = ballistics_register_builtin_writers(writer_registry)) != BALLISTICS_STATUS_OK)
    {
        report_error("register built-ins", status);
        goto cleanup;
    }

    {
        const BallisticsProjectile projectile = {0.018, 0.009, 6.3617e-5};
        const BallisticsLaunchState launch = {{0.0, 0.0, 1.5}, {1.0, 0.0, 0.08}, 310.0};
        const BallisticsLauncherMetadata metadata = {
            "synthetic-research-launcher.v1", "Synthetic Research Launcher", 0.04, 0.0};
        const BallisticsConstantEnvironmentConfig environment_config = {
            1.225, {0.0, 2.0, 0.0}};
        const BallisticsBasicDragConfig drag_config = {0.29};
        const BallisticsGroundStopConfig ground_config = {0.0};
        const BallisticsMaximumTimeStopConfig maximum_time_config = {options.maximum_time_s};
        const BallisticsMaximumDistanceStopConfig maximum_distance_config = {5000.0};
        const BallisticsInvalidStateStopConfig invalid_state_config = {0U};
        BallisticsProjectileState initial_state;
        BallisticsSimulationConfig simulation_config = {
            options.time_step_s, options.maximum_time_s, 5000.0, 0.0, options.integrator_id};

        status = ballistics_projectile_validate(&projectile);
        if (status == BALLISTICS_STATUS_OK)
        {
            status = ballistics_launcher_metadata_validate(&metadata);
        }
        if (status == BALLISTICS_STATUS_OK)
        {
            status = ballistics_launch_state_to_projectile_state(&launch, &initial_state);
        }
        if (status == BALLISTICS_STATUS_OK)
        {
            status = ballistics_simulation_config_validate(&simulation_config, integrator_registry);
        }
        if (status != BALLISTICS_STATUS_OK)
        {
            report_error("validate synthetic scenario", status);
            goto cleanup;
        }
        status = ballistics_constant_environment_model_create(&environment_config, &environment);
        if (status != BALLISTICS_STATUS_OK)
        {
            report_error("create environment", status);
            goto cleanup;
        }
        status = ballistics_force_model_registry_create_instance(
            force_registry, BALLISTICS_CONSTANT_GRAVITY_MODEL_ID, NULL, 0U, &gravity);
        if (status == BALLISTICS_STATUS_OK)
        {
            status = ballistics_force_model_registry_create_instance(
                force_registry,
                BALLISTICS_BASIC_DRAG_MODEL_ID,
                &drag_config,
                sizeof(drag_config),
                &drag);
        }
        if (status != BALLISTICS_STATUS_OK)
        {
            report_error("create force models", status);
            goto cleanup;
        }
        force_models[0] = gravity;
        force_models[1] = drag;
        status = ballistics_integrator_registry_create_instance(
            integrator_registry, options.integrator_id, NULL, 0U, &integrator);
        if (status != BALLISTICS_STATUS_OK)
        {
            report_error("create integrator", status);
            goto cleanup;
        }
        status = ballistics_invalid_state_stop_condition_create(
            &invalid_state_config, &stop_conditions[0]);
        if (status == BALLISTICS_STATUS_OK)
        {
            status = ballistics_ground_stop_condition_create(&ground_config, &stop_conditions[1]);
        }
        if (status == BALLISTICS_STATUS_OK)
        {
            status = ballistics_maximum_distance_stop_condition_create(
                &maximum_distance_config, &stop_conditions[2]);
        }
        if (status == BALLISTICS_STATUS_OK)
        {
            status = ballistics_maximum_time_stop_condition_create(
                &maximum_time_config, &stop_conditions[3]);
        }
        if (status != BALLISTICS_STATUS_OK)
        {
            report_error("create stop conditions", status);
            goto cleanup;
        }
        status = ballistics_dynamics_create(
            &projectile, environment, force_models, BALLISTICS_CLI_FORCE_MODEL_COUNT, &dynamics);
        if (status != BALLISTICS_STATUS_OK)
        {
            report_error("create dynamics", status);
            goto cleanup;
        }
        status = ballistics_simulation_create(&simulation_config,
                                              &initial_state,
                                              dynamics,
                                              integrator,
                                              stop_conditions,
                                              BALLISTICS_CLI_STOP_CONDITION_COUNT,
                                              &simulation);
        if (status != BALLISTICS_STATUS_OK)
        {
            report_error("create simulation", status);
            goto cleanup;
        }
        status = ballistics_simulation_result_create(BALLISTICS_CLI_RESULT_CAPACITY, &result);
        if (status != BALLISTICS_STATUS_OK)
        {
            report_error("create result", status);
            goto cleanup;
        }

        output_file = fopen(options.output_path, "wb");
        if (output_file == NULL)
        {
            fputs("ballistics_cli: unable to open output file\n", stderr);
            goto cleanup;
        }
        {
            const BallisticsCsvWriterConfig writer_config = {
                ballistics_cli_file_byte_sink(output_file)};
            status = ballistics_writer_registry_create_instance(writer_registry,
                                                                BALLISTICS_CSV_WRITER_ID,
                                                                &writer_config,
                                                                sizeof(writer_config),
                                                                &writer);
        }
        if (status != BALLISTICS_STATUS_OK)
        {
            report_error("create CSV writer", status);
            goto cleanup;
        }
        status = ballistics_simulation_run(simulation, result);
        if (status == BALLISTICS_STATUS_OK)
        {
            status = ballistics_trajectory_writer_write_result(writer, result);
        }
        if (status != BALLISTICS_STATUS_OK)
        {
            report_error("run or write simulation", status);
            goto cleanup;
        }
    }

    {
        BallisticsStopReason reason;
        BallisticsProjectileState final_state;
        double final_time_s;
        double final_speed_mps;
        size_t sample_count;

        status = ballistics_simulation_result_stop_reason(result, &reason);
        if (status == BALLISTICS_STATUS_OK)
        {
            status = ballistics_simulation_result_final_time(result, &final_time_s);
        }
        if (status == BALLISTICS_STATUS_OK)
        {
            status = ballistics_simulation_result_final_state(result, &final_state);
        }
        if (status == BALLISTICS_STATUS_OK)
        {
            status = ballistics_vector3_magnitude(&final_state.velocity_mps, &final_speed_mps);
        }
        if (status == BALLISTICS_STATUS_OK)
        {
            status = ballistics_simulation_result_sample_count(result, &sample_count);
        }
        if (status != BALLISTICS_STATUS_OK)
        {
            report_error("read final result", status);
            goto cleanup;
        }
        printf("stop=%s time=%.6f s range=(%.3f, %.3f) m speed=%.3f m/s samples=%zu "
               "output=%s\n",
               stop_reason_name(reason),
               final_time_s,
               final_state.position_m.x,
               final_state.position_m.y,
               final_speed_mps,
               sample_count,
               options.output_path);
    }
    exit_code = EXIT_SUCCESS;

cleanup:
    ballistics_trajectory_writer_destroy(writer);
    if (output_file != NULL && fclose(output_file) != 0 && exit_code == EXIT_SUCCESS)
    {
        fputs("ballistics_cli: unable to close output file\n", stderr);
        exit_code = EXIT_FAILURE;
    }
    ballistics_simulation_result_destroy(result);
    ballistics_simulation_destroy(simulation);
    ballistics_dynamics_destroy(dynamics);
    for (size_t index = 0U; index < BALLISTICS_CLI_STOP_CONDITION_COUNT; ++index)
    {
        ballistics_stop_condition_destroy(stop_conditions[index]);
    }
    ballistics_integrator_destroy(integrator);
    ballistics_force_model_destroy(drag);
    ballistics_force_model_destroy(gravity);
    ballistics_environment_destroy(environment);
    ballistics_writer_registry_destroy(writer_registry);
    ballistics_integrator_registry_destroy(integrator_registry);
    ballistics_force_model_registry_destroy(force_registry);
    ballistics_equation_registry_destroy(equation_registry);
    return exit_code;
}
