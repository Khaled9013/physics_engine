#include "test_scenario.h"

#include <stddef.h>

BallisticsTestScenarioConfig ballistics_test_scenario_defaults(void)
{
    BallisticsTestScenarioConfig config;
    config.integrator_id = BALLISTICS_RK4_INTEGRATOR_ID;
    config.time_step_s = 0.01;
    config.maximum_time_s = 1.0;
    config.maximum_distance_m = 1000000.0;
    config.ground_height_m = -1000.0;
    config.gravity_mps2 = (BallisticsVector3){0.0, 0.0, -9.80665};
    config.density_kgpm3 = 0.0;
    config.wind_mps = (BallisticsVector3){0.0, 0.0, 0.0};
    config.drag_coefficient = 0.0;
    config.projectile = (BallisticsProjectile){1.0, 0.01, 0.01};
    config.initial_state = (BallisticsProjectileState){{0.0, 0.0, 10.0}, {20.0, 0.0, 30.0}};
    return config;
}

void ballistics_test_scenario_destroy(BallisticsTestScenario *scenario)
{
    size_t index;
    if (scenario == NULL)
    {
        return;
    }
    ballistics_simulation_result_destroy(scenario->result);
    ballistics_simulation_destroy(scenario->simulation);
    ballistics_dynamics_destroy(scenario->dynamics);
    for (index = 0U; index < 4U; ++index)
    {
        ballistics_stop_condition_destroy(scenario->stops[index]);
    }
    ballistics_integrator_destroy(scenario->integrator);
    ballistics_integrator_registry_destroy(scenario->integrator_registry);
    ballistics_force_model_destroy(scenario->drag);
    ballistics_force_model_destroy(scenario->gravity);
    ballistics_environment_destroy(scenario->environment);
    *scenario = (BallisticsTestScenario){0};
}

BallisticsStatus ballistics_test_scenario_run(const BallisticsTestScenarioConfig *config,
                                              BallisticsTestScenario *out_scenario)
{
    BallisticsConstantEnvironmentConfig environment_config;
    BallisticsConstantGravityConfig gravity_config;
    BallisticsBasicDragConfig drag_config;
    BallisticsInvalidStateStopConfig invalid_config = {0U};
    BallisticsGroundStopConfig ground_config;
    BallisticsMaximumDistanceStopConfig distance_config;
    BallisticsMaximumTimeStopConfig time_config;
    BallisticsSimulationConfig simulation_config;
    BallisticsStatus status;

    if (config == NULL || out_scenario == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    *out_scenario = (BallisticsTestScenario){0};
    out_scenario->projectile = config->projectile;
    environment_config.air_density_kgpm3 = config->density_kgpm3;
    environment_config.wind_velocity_mps = config->wind_mps;
    gravity_config.acceleration_mps2 = config->gravity_mps2;
    drag_config.drag_coefficient = config->drag_coefficient;
    ground_config.ground_height_m = config->ground_height_m;
    distance_config.maximum_horizontal_distance_m = config->maximum_distance_m;
    time_config.maximum_time_s = config->maximum_time_s;
    simulation_config = (BallisticsSimulationConfig){config->time_step_s,
                                                     config->maximum_time_s,
                                                     config->maximum_distance_m,
                                                     config->ground_height_m,
                                                     config->integrator_id};

    status = ballistics_constant_environment_model_create(
        &environment_config, &out_scenario->environment);
    if (status == BALLISTICS_STATUS_OK)
    {
        status = ballistics_constant_gravity_model_create(&gravity_config, &out_scenario->gravity);
    }
    if (status == BALLISTICS_STATUS_OK)
    {
        status = ballistics_basic_drag_model_create(&drag_config, &out_scenario->drag);
    }
    if (status == BALLISTICS_STATUS_OK)
    {
        status = ballistics_integrator_registry_create(4U, &out_scenario->integrator_registry);
    }
    if (status == BALLISTICS_STATUS_OK)
    {
        status = ballistics_register_builtin_integrators(out_scenario->integrator_registry);
    }
    if (status == BALLISTICS_STATUS_OK)
    {
        status = ballistics_simulation_config_validate(
            &simulation_config, out_scenario->integrator_registry);
    }
    if (status == BALLISTICS_STATUS_OK)
    {
        status = ballistics_integrator_registry_create_instance(out_scenario->integrator_registry,
                                                                config->integrator_id,
                                                                NULL,
                                                                0U,
                                                                &out_scenario->integrator);
    }
    if (status == BALLISTICS_STATUS_OK)
    {
        status = ballistics_invalid_state_stop_condition_create(
            &invalid_config, &out_scenario->stops[0]);
    }
    if (status == BALLISTICS_STATUS_OK)
    {
        status = ballistics_ground_stop_condition_create(&ground_config, &out_scenario->stops[1]);
    }
    if (status == BALLISTICS_STATUS_OK)
    {
        status = ballistics_maximum_distance_stop_condition_create(
            &distance_config, &out_scenario->stops[2]);
    }
    if (status == BALLISTICS_STATUS_OK)
    {
        status = ballistics_maximum_time_stop_condition_create(
            &time_config, &out_scenario->stops[3]);
    }
    out_scenario->forces[0] = out_scenario->gravity;
    out_scenario->forces[1] = out_scenario->drag;
    if (status == BALLISTICS_STATUS_OK)
    {
        status = ballistics_dynamics_create(&out_scenario->projectile,
                                            out_scenario->environment,
                                            out_scenario->forces,
                                            2U,
                                            &out_scenario->dynamics);
    }
    if (status == BALLISTICS_STATUS_OK)
    {
        status = ballistics_simulation_create(&simulation_config,
                                              &config->initial_state,
                                              out_scenario->dynamics,
                                              out_scenario->integrator,
                                              out_scenario->stops,
                                              4U,
                                              &out_scenario->simulation);
    }
    if (status == BALLISTICS_STATUS_OK)
    {
        status = ballistics_simulation_result_create(4U, &out_scenario->result);
    }
    if (status == BALLISTICS_STATUS_OK)
    {
        status = ballistics_simulation_run(out_scenario->simulation, out_scenario->result);
    }
    if (status != BALLISTICS_STATUS_OK)
    {
        ballistics_test_scenario_destroy(out_scenario);
    }
    return status;
}

BallisticsStatus ballistics_test_scenario_final_sample(
    const BallisticsTestScenario *scenario,
    const BallisticsTrajectorySample **out_sample)
{
    size_t count;
    BallisticsStatus status;
    if (scenario == NULL || out_sample == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    status = ballistics_simulation_result_sample_count(scenario->result, &count);
    if (status != BALLISTICS_STATUS_OK || count == 0U)
    {
        return status == BALLISTICS_STATUS_OK ? BALLISTICS_STATUS_NOT_FOUND : status;
    }
    return ballistics_simulation_result_sample_at(scenario->result, count - 1U, out_sample);
}
