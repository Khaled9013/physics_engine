#include "ballistics/simulation.h"

#include "ballistics/port/ballistics_port.h"
#include "simulation_result_internal.h"

#include <math.h>
#include <stddef.h>

#define BALLISTICS_SIMULATION_STATE_COUNT 6U

struct BallisticsSimulation
{
    BallisticsSimulationConfig config;
    BallisticsProjectileState initial_state;
    BallisticsDynamicsContext *dynamics;
    BallisticsIntegrator *integrator;
    BallisticsStopCondition *const *stop_conditions;
    size_t stop_condition_count;
};

static void projectile_state_to_values(const BallisticsProjectileState *state,
                                       double values[BALLISTICS_SIMULATION_STATE_COUNT])
{
    values[0] = state->position_m.x;
    values[1] = state->position_m.y;
    values[2] = state->position_m.z;
    values[3] = state->velocity_mps.x;
    values[4] = state->velocity_mps.y;
    values[5] = state->velocity_mps.z;
}

static BallisticsProjectileState values_to_projectile_state(
    const double values[BALLISTICS_SIMULATION_STATE_COUNT])
{
    BallisticsProjectileState state;
    state.position_m = (BallisticsVector3){values[0], values[1], values[2]};
    state.velocity_mps = (BallisticsVector3){values[3], values[4], values[5]};
    return state;
}

static int stop_reason_priority(BallisticsStopReason reason)
{
    switch (reason)
    {
        case BALLISTICS_STOP_REASON_INVALID_STATE:
            return 0;
        case BALLISTICS_STOP_REASON_GROUND_INTERSECTION:
            return 1;
        case BALLISTICS_STOP_REASON_MAXIMUM_DISTANCE:
            return 2;
        case BALLISTICS_STOP_REASON_MAXIMUM_TIME:
            return 3;
        default:
            return 4;
    }
}

static BallisticsStatus append_sample(BallisticsSimulation *simulation,
                                      BallisticsSimulationResult *result,
                                      double time_s,
                                      const BallisticsProjectileState *state)
{
    BallisticsTrajectorySample sample;
    BallisticsStatus status;

    sample.time_s = time_s;
    sample.state = *state;
    status = ballistics_dynamics_acceleration(
        simulation->dynamics, time_s, state, &sample.acceleration_mps2);
    if (status != BALLISTICS_STATUS_OK)
    {
        return status;
    }
    return ballistics_simulation_result_append(result, &sample);
}

static BallisticsStatus evaluate_stop_conditions(
    const BallisticsSimulation *simulation,
    const BallisticsStopEvaluation *evaluation,
    BallisticsStopDecision *out_decision)
{
    size_t index;
    int selected_priority = 4;

    out_decision->stop = false;
    out_decision->reason = BALLISTICS_STOP_REASON_NONE;
    for (index = 0U; index < simulation->stop_condition_count; ++index)
    {
        BallisticsStopDecision candidate;
        const BallisticsStatus status = ballistics_stop_condition_evaluate(
            simulation->stop_conditions[index], evaluation, &candidate);
        if (status != BALLISTICS_STATUS_OK)
        {
            return status;
        }
        if (candidate.stop && stop_reason_priority(candidate.reason) < selected_priority)
        {
            *out_decision = candidate;
            selected_priority = stop_reason_priority(candidate.reason);
        }
    }
    return BALLISTICS_STATUS_OK;
}

BallisticsStatus ballistics_simulation_create(
    const BallisticsSimulationConfig *config,
    const BallisticsProjectileState *initial_state,
    BallisticsDynamicsContext *dynamics,
    BallisticsIntegrator *integrator,
    BallisticsStopCondition *const *stop_conditions,
    size_t stop_condition_count,
    BallisticsSimulation **out_simulation)
{
    BallisticsSimulation *simulation = NULL;
    void *memory = NULL;
    size_t index;
    BallisticsStatus status;

    if (out_simulation == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    *out_simulation = NULL;
    status = ballistics_simulation_config_validate(config, NULL);
    if (status != BALLISTICS_STATUS_OK ||
        ballistics_projectile_state_validate(initial_state) != BALLISTICS_STATUS_OK ||
        dynamics == NULL || integrator == NULL || integrator->vtable == NULL ||
        integrator->vtable->step == NULL || stop_conditions == NULL || stop_condition_count == 0U)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    for (index = 0U; index < stop_condition_count; ++index)
    {
        if (stop_conditions[index] == NULL || stop_conditions[index]->vtable == NULL ||
            stop_conditions[index]->vtable->evaluate == NULL)
        {
            return BALLISTICS_STATUS_INVALID_ARGUMENT;
        }
    }
    status = ballistics_port_allocate(sizeof(*simulation), &memory);
    if (status != BALLISTICS_STATUS_OK)
    {
        return status;
    }
    simulation = memory;
    simulation->config = *config;
    simulation->initial_state = *initial_state;
    simulation->dynamics = dynamics;
    simulation->integrator = integrator;
    simulation->stop_conditions = stop_conditions;
    simulation->stop_condition_count = stop_condition_count;
    *out_simulation = simulation;
    return BALLISTICS_STATUS_OK;
}

void ballistics_simulation_destroy(BallisticsSimulation *simulation)
{
    (void)ballistics_port_deallocate(simulation);
}

BallisticsStatus ballistics_simulation_run(BallisticsSimulation *simulation,
                                           BallisticsSimulationResult *result)
{
    BallisticsProjectileState current_state;
    double current_values[BALLISTICS_SIMULATION_STATE_COUNT];
    double next_values[BALLISTICS_SIMULATION_STATE_COUNT];
    double current_time_s = 0.0;
    BallisticsStatus status;

    if (simulation == NULL || result == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    status = ballistics_simulation_result_clear(result);
    if (status != BALLISTICS_STATUS_OK)
    {
        return status;
    }
    current_state = simulation->initial_state;
    projectile_state_to_values(&current_state, current_values);
    status = append_sample(simulation, result, current_time_s, &current_state);
    if (status != BALLISTICS_STATUS_OK)
    {
        return status;
    }

    for (;;)
    {
        BallisticsProjectileState next_state;
        BallisticsStopEvaluation evaluation;
        BallisticsStopDecision decision;
        double remaining_time_s = simulation->config.maximum_time_seconds - current_time_s;
        double time_step_s = fmin(simulation->config.time_step_seconds, remaining_time_s);
        double next_time_s;

        if (!isfinite(time_step_s) || time_step_s <= 0.0)
        {
            return ballistics_simulation_result_set_stop(result,
                                                         BALLISTICS_STOP_REASON_MAXIMUM_TIME,
                                                         current_time_s,
                                                         &current_state);
        }
        next_time_s = current_time_s + time_step_s;
        if (!isfinite(next_time_s) || next_time_s <= current_time_s)
        {
            return BALLISTICS_STATUS_NUMERICAL_ERROR;
        }
        status = ballistics_integrator_step(simulation->integrator,
                                            current_values,
                                            BALLISTICS_SIMULATION_STATE_COUNT,
                                            current_time_s,
                                            time_step_s,
                                            ballistics_dynamics_derivative,
                                            simulation->dynamics,
                                            next_values);
        if (status == BALLISTICS_STATUS_NUMERICAL_ERROR)
        {
            return ballistics_simulation_result_set_stop(result,
                                                         BALLISTICS_STOP_REASON_INVALID_STATE,
                                                         current_time_s,
                                                         &current_state);
        }
        if (status != BALLISTICS_STATUS_OK)
        {
            return status;
        }
        next_state = values_to_projectile_state(next_values);
        evaluation.previous_time_s = current_time_s;
        evaluation.previous_state = &current_state;
        evaluation.current_time_s = next_time_s;
        evaluation.current_state = &next_state;
        evaluation.initial_state = &simulation->initial_state;
        status = evaluate_stop_conditions(simulation, &evaluation, &decision);
        if (status != BALLISTICS_STATUS_OK)
        {
            return status;
        }
        if (!decision.stop && next_time_s >= simulation->config.maximum_time_seconds)
        {
            decision.stop = true;
            decision.reason = BALLISTICS_STOP_REASON_MAXIMUM_TIME;
            decision.final_time_s = next_time_s;
            decision.final_state = next_state;
        }
        if (decision.stop)
        {
            status = append_sample(
                simulation, result, decision.final_time_s, &decision.final_state);
            if (status != BALLISTICS_STATUS_OK)
            {
                return status;
            }
            return ballistics_simulation_result_set_stop(result,
                                                         decision.reason,
                                                         decision.final_time_s,
                                                         &decision.final_state);
        }
        status = append_sample(simulation, result, next_time_s, &next_state);
        if (status != BALLISTICS_STATUS_OK)
        {
            return status;
        }
        current_time_s = next_time_s;
        current_state = next_state;
        projectile_state_to_values(&current_state, current_values);
    }
}
