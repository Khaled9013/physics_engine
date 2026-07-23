#include "ballistics/dynamics.h"

#include "ballistics/equations/acceleration_equation.h"
#include "ballistics/port/ballistics_port.h"

#include <math.h>
#include <stddef.h>

#define BALLISTICS_PROJECTILE_STATE_VALUE_COUNT 6U

struct BallisticsDynamicsContext
{
    const BallisticsProjectile *projectile;
    const BallisticsEnvironmentModel *environment;
    BallisticsForceModel *const *force_models;
    size_t force_model_count;
};

BallisticsStatus ballistics_dynamics_create(const BallisticsProjectile *projectile,
                                            const BallisticsEnvironmentModel *environment,
                                            BallisticsForceModel *const *force_models,
                                            size_t force_model_count,
                                            BallisticsDynamicsContext **out_dynamics)
{
    BallisticsDynamicsContext *dynamics = NULL;
    void *memory = NULL;
    size_t index;
    BallisticsStatus status;

    if (out_dynamics == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    *out_dynamics = NULL;
    status = ballistics_projectile_validate(projectile);
    if (status != BALLISTICS_STATUS_OK || environment == NULL || environment->vtable == NULL ||
        environment->vtable->sample == NULL || force_models == NULL || force_model_count == 0U)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    for (index = 0U; index < force_model_count; ++index)
    {
        if (force_models[index] == NULL || force_models[index]->vtable == NULL ||
            force_models[index]->vtable->calculate_force == NULL)
        {
            return BALLISTICS_STATUS_INVALID_ARGUMENT;
        }
    }
    status = ballistics_port_allocate(sizeof(*dynamics), &memory);
    if (status != BALLISTICS_STATUS_OK)
    {
        return status;
    }
    dynamics = memory;
    dynamics->projectile = projectile;
    dynamics->environment = environment;
    dynamics->force_models = force_models;
    dynamics->force_model_count = force_model_count;
    *out_dynamics = dynamics;
    return BALLISTICS_STATUS_OK;
}

void ballistics_dynamics_destroy(BallisticsDynamicsContext *dynamics)
{
    (void)ballistics_port_deallocate(dynamics);
}

BallisticsStatus ballistics_dynamics_acceleration(const BallisticsDynamicsContext *dynamics,
                                                 double time_s,
                                                 const BallisticsProjectileState *state,
                                                 BallisticsVector3 *out_acceleration_mps2)
{
    BallisticsEnvironmentState environment_state;
    BallisticsVector3 total_force_n = {0.0, 0.0, 0.0};
    BallisticsAccelerationInput acceleration_input;
    size_t index;
    BallisticsStatus status;

    if (dynamics == NULL || !isfinite(time_s) || out_acceleration_mps2 == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    status = ballistics_projectile_state_validate(state);
    if (status != BALLISTICS_STATUS_OK)
    {
        return status;
    }
    status = ballistics_environment_sample(
        dynamics->environment, time_s, &state->position_m, &environment_state);
    if (status != BALLISTICS_STATUS_OK)
    {
        return status;
    }
    for (index = 0U; index < dynamics->force_model_count; ++index)
    {
        BallisticsVector3 force_n;
        status = ballistics_force_model_calculate_force(dynamics->force_models[index],
                                                        dynamics->projectile,
                                                        state,
                                                        &environment_state,
                                                        &force_n);
        if (status != BALLISTICS_STATUS_OK)
        {
            return status;
        }
        status = ballistics_vector3_add(&total_force_n, &force_n, &total_force_n);
        if (status != BALLISTICS_STATUS_OK)
        {
            return status;
        }
    }
    acceleration_input.total_force_n = total_force_n;
    acceleration_input.mass_kg = dynamics->projectile->mass_kg;
    return ballistics_acceleration_evaluate(&acceleration_input, out_acceleration_mps2);
}

BallisticsStatus ballistics_dynamics_derivative(double time_s,
                                                const double *state,
                                                size_t state_count,
                                                double *out_derivative,
                                                void *context)
{
    BallisticsProjectileState projectile_state;
    BallisticsVector3 acceleration_mps2;
    BallisticsStatus status;

    if (state == NULL || out_derivative == NULL || context == NULL ||
        state_count != BALLISTICS_PROJECTILE_STATE_VALUE_COUNT)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    projectile_state.position_m = (BallisticsVector3){state[0], state[1], state[2]};
    projectile_state.velocity_mps = (BallisticsVector3){state[3], state[4], state[5]};
    status = ballistics_dynamics_acceleration(context, time_s, &projectile_state, &acceleration_mps2);
    if (status != BALLISTICS_STATUS_OK)
    {
        return status;
    }
    out_derivative[0] = projectile_state.velocity_mps.x;
    out_derivative[1] = projectile_state.velocity_mps.y;
    out_derivative[2] = projectile_state.velocity_mps.z;
    out_derivative[3] = acceleration_mps2.x;
    out_derivative[4] = acceleration_mps2.y;
    out_derivative[5] = acceleration_mps2.z;
    return BALLISTICS_STATUS_OK;
}
