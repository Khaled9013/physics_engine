#ifndef BALLISTICS_TEST_SCENARIO_H
#define BALLISTICS_TEST_SCENARIO_H

#include "ballistics/ballistics.h"

typedef struct
{
    const char *integrator_id;
    double time_step_s;
    double maximum_time_s;
    double maximum_distance_m;
    double ground_height_m;
    BallisticsVector3 gravity_mps2;
    double density_kgpm3;
    BallisticsVector3 wind_mps;
    double drag_coefficient;
    BallisticsProjectile projectile;
    BallisticsProjectileState initial_state;
} BallisticsTestScenarioConfig;

typedef struct
{
    BallisticsProjectile projectile;
    BallisticsEnvironmentModel *environment;
    BallisticsForceModel *gravity;
    BallisticsForceModel *drag;
    BallisticsForceModel *forces[2];
    BallisticsIntegratorRegistry *integrator_registry;
    BallisticsIntegrator *integrator;
    BallisticsStopCondition *stops[4];
    BallisticsDynamicsContext *dynamics;
    BallisticsSimulation *simulation;
    BallisticsSimulationResult *result;
} BallisticsTestScenario;

BallisticsTestScenarioConfig ballistics_test_scenario_defaults(void);
BallisticsStatus ballistics_test_scenario_run(const BallisticsTestScenarioConfig *config,
                                              BallisticsTestScenario *out_scenario);
void ballistics_test_scenario_destroy(BallisticsTestScenario *scenario);
BallisticsStatus ballistics_test_scenario_final_sample(
    const BallisticsTestScenario *scenario,
    const BallisticsTrajectorySample **out_sample);

#endif
