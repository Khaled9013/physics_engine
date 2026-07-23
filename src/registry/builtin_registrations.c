#include "ballistics/registry/builtin_registry.h"

#include "ballistics/equations/acceleration_equation.h"
#include "ballistics/equations/aerodynamic_drag_equation.h"
#include "ballistics/equations/air_relative_velocity_equation.h"
#include "ballistics/models/constant_gravity_model.h"
#include "ballistics/models/basic_drag_model.h"
#include "ballistics/integrators/euler_integrator.h"
#include "ballistics/integrators/rk4_integrator.h"
#include "ballistics/output/csv_trajectory_writer.h"

BallisticsStatus ballistics_register_builtin_equations(BallisticsEquationRegistry *registry)
{
    static const struct
    {
        const char *identifier;
        BallisticsEquationFactory factory;
    } registrations[] = {
        {BALLISTICS_AIR_RELATIVE_VELOCITY_EQUATION_ID,
         ballistics_air_relative_velocity_equation_factory},
        {BALLISTICS_AERODYNAMIC_DRAG_EQUATION_ID, ballistics_aerodynamic_drag_equation_factory},
        {BALLISTICS_ACCELERATION_EQUATION_ID, ballistics_acceleration_equation_factory},
    };
    size_t index;

    if (registry == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    for (index = 0U; index < sizeof(registrations) / sizeof(registrations[0]); ++index)
    {
        const BallisticsStatus status = ballistics_equation_registry_register(
            registry, registrations[index].identifier, registrations[index].factory);
        if (status != BALLISTICS_STATUS_OK)
        {
            return status;
        }
    }
    return BALLISTICS_STATUS_OK;
}


BallisticsStatus ballistics_register_builtin_force_models(BallisticsForceModelRegistry *registry)
{
    static const struct
    {
        const char *identifier;
        BallisticsForceModelFactory factory;
    } registrations[] = {
        {BALLISTICS_CONSTANT_GRAVITY_MODEL_ID, ballistics_constant_gravity_model_factory},
        {BALLISTICS_BASIC_DRAG_MODEL_ID, ballistics_basic_drag_model_factory},
    };
    size_t index;

    if (registry == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    for (index = 0U; index < sizeof(registrations) / sizeof(registrations[0]); ++index)
    {
        const BallisticsStatus status = ballistics_force_model_registry_register(
            registry, registrations[index].identifier, registrations[index].factory);
        if (status != BALLISTICS_STATUS_OK)
        {
            return status;
        }
    }
    return BALLISTICS_STATUS_OK;
}


BallisticsStatus ballistics_register_builtin_integrators(BallisticsIntegratorRegistry *registry)
{
    static const struct
    {
        const char *identifier;
        BallisticsIntegratorFactory factory;
    } registrations[] = {
        {BALLISTICS_EULER_INTEGRATOR_ID, ballistics_euler_integrator_factory},
        {BALLISTICS_RK4_INTEGRATOR_ID, ballistics_rk4_integrator_factory},
    };
    size_t index;

    if (registry == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    for (index = 0U; index < sizeof(registrations) / sizeof(registrations[0]); ++index)
    {
        const BallisticsStatus status = ballistics_integrator_registry_register(
            registry, registrations[index].identifier, registrations[index].factory);
        if (status != BALLISTICS_STATUS_OK)
        {
            return status;
        }
    }
    return BALLISTICS_STATUS_OK;
}


BallisticsStatus ballistics_register_builtin_writers(BallisticsWriterRegistry *registry)
{
    if (registry == NULL)
    {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    return ballistics_writer_registry_register(
        registry, BALLISTICS_CSV_WRITER_ID, ballistics_csv_trajectory_writer_factory);
}
