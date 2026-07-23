#include "ballistics/ballistics.h"

#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    const BallisticsProjectile projectile = {1.0, 0.01, 0.01};
    const BallisticsProjectileState initial = {{0.0, 0.0, 10.0}, {20.0, 0.0, 5.0}};
    const BallisticsConstantEnvironmentConfig environment_config = {0.0, {0.0, 0.0, 0.0}};
    const BallisticsConstantGravityConfig gravity_config = {{0.0, 0.0, -9.80665}};
    const BallisticsBasicDragConfig drag_config = {0.0};
    const BallisticsRk4IntegratorConfig integrator_config = {6U};
    const BallisticsInvalidStateStopConfig invalid_config = {0U};
    const BallisticsGroundStopConfig ground_config = {-1000.0};
    const BallisticsMaximumDistanceStopConfig distance_config = {1000.0};
    const BallisticsMaximumTimeStopConfig time_config = {1.0};
    const BallisticsSimulationConfig simulation_config = {0.01, 1.0, 1000.0, -1000.0, "rk4.v1"};
    BallisticsEnvironmentModel *environment = NULL;
    BallisticsForceModel *gravity = NULL;
    BallisticsForceModel *drag = NULL;
    BallisticsForceModel *forces[2];
    BallisticsIntegrator *integrator = NULL;
    BallisticsStopCondition *stops[4] = {NULL};
    BallisticsDynamicsContext *dynamics = NULL;
    BallisticsSimulation *simulation = NULL;
    BallisticsSimulationResult *result = NULL;
    BallisticsProjectileState final_state;
    double final_time_s;
    BallisticsStatus status;
    int exit_code = EXIT_FAILURE;

    status = ballistics_constant_environment_model_create(&environment_config, &environment);
    if (status == BALLISTICS_STATUS_OK)
        status = ballistics_constant_gravity_model_create(&gravity_config, &gravity);
    if (status == BALLISTICS_STATUS_OK)
        status = ballistics_basic_drag_model_create(&drag_config, &drag);
    if (status == BALLISTICS_STATUS_OK)
        status = ballistics_rk4_integrator_create(&integrator_config, &integrator);
    if (status == BALLISTICS_STATUS_OK)
        status = ballistics_invalid_state_stop_condition_create(&invalid_config, &stops[0]);
    if (status == BALLISTICS_STATUS_OK)
        status = ballistics_ground_stop_condition_create(&ground_config, &stops[1]);
    if (status == BALLISTICS_STATUS_OK)
        status = ballistics_maximum_distance_stop_condition_create(&distance_config, &stops[2]);
    if (status == BALLISTICS_STATUS_OK)
        status = ballistics_maximum_time_stop_condition_create(&time_config, &stops[3]);
    forces[0] = gravity;
    forces[1] = drag;
    if (status == BALLISTICS_STATUS_OK)
        status = ballistics_dynamics_create(&projectile, environment, forces, 2U, &dynamics);
    if (status == BALLISTICS_STATUS_OK)
        status = ballistics_simulation_create(
            &simulation_config, &initial, dynamics, integrator, stops, 4U, &simulation);
    if (status == BALLISTICS_STATUS_OK)
        status = ballistics_simulation_result_create(8U, &result);
    if (status == BALLISTICS_STATUS_OK)
        status = ballistics_simulation_run(simulation, result);
    if (status == BALLISTICS_STATUS_OK)
        status = ballistics_simulation_result_final_time(result, &final_time_s);
    if (status == BALLISTICS_STATUS_OK)
        status = ballistics_simulation_result_final_state(result, &final_state);
    if (status == BALLISTICS_STATUS_OK)
    {
        printf("t=%.3f s position=(%.6f, %.6f, %.6f) m\n",
               final_time_s,
               final_state.position_m.x,
               final_state.position_m.y,
               final_state.position_m.z);
        exit_code = EXIT_SUCCESS;
    }
    else
    {
        fprintf(stderr, "basic scenario failed: %s\n", ballistics_status_to_string(status));
    }

    ballistics_simulation_result_destroy(result);
    ballistics_simulation_destroy(simulation);
    ballistics_dynamics_destroy(dynamics);
    for (size_t index = 0U; index < 4U; ++index)
        ballistics_stop_condition_destroy(stops[index]);
    ballistics_integrator_destroy(integrator);
    ballistics_force_model_destroy(drag);
    ballistics_force_model_destroy(gravity);
    ballistics_environment_destroy(environment);
    return exit_code;
}
