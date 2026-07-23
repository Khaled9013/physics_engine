# Frozen Public Interface Contracts

Contract version: Phase One C API v1, frozen 2026-07-23. This document is normative. Public declarations live under `include/ballistics/`; private implementation headers stay under `src/`. Enum values and stable identifiers are never renumbered or repurposed; extensions append values.

## Common rules

- Public symbols use `ballistics_`, types use `Ballistics`, and macros use `BALLISTICS_`.
- Required pointers and externally supplied values are validated. Null required pointers return `BALLISTICS_STATUS_INVALID_ARGUMENT`.
- Library code never exits and never prints outside the debug interface.
- Distinct instances may run concurrently. Concurrent mutation/use of the same registry, result, writer, dynamics, or simulation requires caller synchronization.
- Values are SI `double`; vectors and states must be finite unless checking validity.
- Create sets `*out = NULL`; destroy accepts `NULL`. Borrowed objects must outlive borrowers.
- Dynamic allocation goes through the port.

## Status, export, and mathematical types

`BallisticsStatus`: `OK`, `INVALID_ARGUMENT`, `NOT_INITIALIZED`, `ALREADY_INITIALIZED`, `NOT_FOUND`, `DUPLICATE`, `OUT_OF_MEMORY`, `IO_ERROR`, `NUMERICAL_ERROR`, `UNSUPPORTED_PLATFORM`, `CAPACITY_EXCEEDED`, `INTERNAL_ERROR`. `ballistics_status_to_string` returns a static string, including `"unknown status"`.

`BALLISTICS_API` controls shared-library symbol visibility. `BallisticsVector3` contains `x,y,z`. Add, subtract, scale, dot, magnitude, normalize, and finite-check operations validate outputs. Normalizing zero returns `INVALID_ARGUMENT`.

`BallisticsProjectile` contains positive finite `mass_kg`, non-negative finite `diameter_m`, positive finite `reference_area_m2`. `BallisticsProjectileState` contains position and velocity. `BallisticsStateDerivative` contains position and velocity derivatives. `BallisticsEnvironmentState` has non-negative finite density and finite wind. `BallisticsLaunchState` has finite position/direction, non-zero direction normalized during conversion, and non-negative speed. `BallisticsLauncherMetadata` has borrowed optional strings and finite reserved sight fields ignored by Phase One.

## Equation interface

```c
typedef struct BallisticsEquation BallisticsEquation;
typedef struct {
    BallisticsStatus (*evaluate)(const BallisticsEquation *self,
        const void *input, size_t input_size, void *output, size_t output_size);
    BallisticsStatus (*initialize)(BallisticsEquation *self); /* optional */
    void (*destroy)(BallisticsEquation *self);
} BallisticsEquationVTable;
struct BallisticsEquation {
    const BallisticsEquationVTable *vtable;
    void *context;
    const char *identifier;
    const char *name;
    BallisticsEquationCategory category;
    const char *input_description;
    const char *output_description;
};
```

The adapter validates exact sizes. Typed callers use family functions. Optional initialize accepts no untyped configuration; typed configuration belongs in typed creation. Categories: KINEMATICS, AERODYNAMICS, ATMOSPHERE, CUSTOM; future SOLVERS and GUIDANCE/TRACKING append.

Typed equations are air-relative velocity input `{projectile_velocity_mps, wind_velocity_mps}` to vector; aerodynamic-drag input `{relative_air_velocity_mps, density, coefficient, area}` to force; acceleration input `{total_force_n, mass_kg}` to acceleration. They are pure/reentrant. Identifiers: `air-relative-velocity.v1`, `basic-aerodynamic-drag.v1`, `acceleration-from-force.v1`.

## Environment interface

```c
typedef struct BallisticsEnvironmentModel BallisticsEnvironmentModel;
typedef struct {
    BallisticsStatus (*sample)(const BallisticsEnvironmentModel *self,
        double time_s, const BallisticsVector3 *position_m,
        BallisticsEnvironmentState *out_state);
    void (*destroy)(BallisticsEnvironmentModel *self);
} BallisticsEnvironmentModelVTable;
```

`BallisticsConstantEnvironmentConfig` contains density and wind and is copied.

## Force, gravity, and drag interfaces

```c
typedef struct BallisticsForceModel BallisticsForceModel;
typedef struct {
    BallisticsStatus (*calculate_force)(const BallisticsForceModel *self,
        const BallisticsProjectile *projectile,
        const BallisticsProjectileState *state,
        const BallisticsEnvironmentState *environment,
        BallisticsVector3 *out_force_n);
    void (*destroy)(BallisticsForceModel *self);
} BallisticsForceModelVTable;
```

`BallisticsGravityModel` and `BallisticsDragModel` are semantic aliases of the force interface, preserving a single engine participant. Calls are read-only and allocation-free. `BallisticsConstantGravityConfig` contains acceleration; `NULL,0` factory default is `[0,0,-9.80665]`, identifier `constant-gravity.v1`. `BallisticsBasicDragConfig` contains a non-negative finite coefficient and invokes the typed equations; identifier `basic-drag.v1`.

## Integrator interface

```c
typedef BallisticsStatus (*BallisticsDerivativeFunction)(
    double time_s, const double *state, size_t state_count,
    double *out_derivative, void *context);
typedef struct BallisticsIntegrator BallisticsIntegrator;
typedef struct {
    BallisticsStatus (*step)(const BallisticsIntegrator *self,
        const double *current_state, size_t state_count,
        double current_time_s, double time_step_s,
        BallisticsDerivativeFunction derivative, void *context,
        double *out_state);
    void (*destroy)(BallisticsIntegrator *self);
} BallisticsIntegratorVTable;
```

(The leading `+` characters in the callback prototype above are prose markers only; the header contains normal declarations.) Input/output may not overlap; callback is synchronous and unretained. Phase One uses positive finite fixed steps. Euler evaluates once, RK4 four times. Both typed configs contain `maximum_state_count`, default 64 for `NULL,0`, identifiers `euler.v1` and `rk4.v1`.

## Dynamics context

`ballistics_dynamics_create` borrows projectile, environment, and an ordered force-model array. Each callback samples environment, sums forces in array order, divides by mass through the acceleration equation, and emits `[vx,vy,vz,ax,ay,az]`. Destroy does not destroy dependencies. No step allocation occurs.

## Stop conditions

```c
typedef struct {
    double previous_time_s;
    const BallisticsProjectileState *previous_state;
    double current_time_s;
    const BallisticsProjectileState *current_state;
    const BallisticsProjectileState *initial_state;
} BallisticsStopEvaluation;
typedef struct {
    bool stop;
    BallisticsStopReason reason;
    double final_time_s;
    BallisticsProjectileState final_state;
} BallisticsStopDecision;
typedef struct BallisticsStopCondition BallisticsStopCondition;
typedef struct {
    BallisticsStatus (*evaluate)(const BallisticsStopCondition *self,
        const BallisticsStopEvaluation *evaluation,
        BallisticsStopDecision *out_decision);
    void (*destroy)(BallisticsStopCondition *self);
} BallisticsStopConditionVTable;
```

Evaluation priority: invalid state, ground, maximum horizontal distance, maximum time. Ground linearly interpolates time, position, velocity. Maximum distance is `hypot(x-x0,y-y0)`; altitude does not count. Typed configs contain ground height, maximum time, or maximum distance; invalid-state uses no config.

## Simulation and result

`BallisticsSimulationConfig` contains positive finite step, maximum time and maximum distance, finite ground height, and borrowed integrator identifier metadata. Validation can check the identifier registry.

`ballistics_simulation_create(config, initial_state, dynamics, integrator, conditions, count, out)` copies config/state and borrows dependencies. It emits the initial sample, shortens a step to max time, evaluates acceleration for emitted states, and replaces a below-ground sample with the interpolated decision. It is sequentially reusable.

`BallisticsTrajectorySample` contains time, state, acceleration. Opaque `BallisticsSimulationResult` owns deterministic controlled-growth samples and final metadata. Queries return borrowed immutable sample pointers valid until clear/destroy/next run.

## Byte sink and writer

```c
typedef struct {
    BallisticsStatus (*write)(void *context, const void *data, size_t size);
    BallisticsStatus (*flush)(void *context);
    void *context;
} BallisticsByteSink;
typedef struct BallisticsTrajectoryWriter BallisticsTrajectoryWriter;
typedef struct {
    BallisticsStatus (*write_result)(BallisticsTrajectoryWriter *self,
        const BallisticsSimulationResult *result);
    void (*destroy)(BallisticsTrajectoryWriter *self);
} BallisticsTrajectoryWriterVTable;
```

Sink context is borrowed and never closed/freed by writer. CSV config copies a sink; output uses the fixed header, `%.9e`, and `\n`. Identifier `csv-writer.v1`. Test memory sink is test support, not core.

## Registries and factory pattern

Four opaque registries expose create/destroy, register, find, count, entry-at, and create-by-id. Registered static identifier/factory lifetimes must exceed the registry. Invalid identifiers/factories, duplicates, and overflow are rejected. Registries do not own created objects.

```c
typedef BallisticsStatus (*BallisticsEquationFactory)(
    const void *config, size_t config_size, BallisticsEquation **out);
typedef BallisticsStatus (*BallisticsForceModelFactory)(
    const void *config, size_t config_size, BallisticsForceModel **out);
typedef BallisticsStatus (*BallisticsIntegratorFactory)(
    const void *config, size_t config_size, BallisticsIntegrator **out);
typedef BallisticsStatus (*BallisticsWriterFactory)(
    const void *config, size_t config_size, BallisticsTrajectoryWriter **out);
```

Factories reject wrong sizes and invalid contents. `NULL,0` is accepted only for documented defaults. Direct typed create functions remain available. Built-in registration is one centralized table line per implementation.

## Debug contract

Levels ERROR, WARNING, INFO, DEBUG, TRACE are most-to-least severe. One documented exceptional process-wide mutable debug configuration stores level/callback and does not affect numerical results; configure it before threads. Logs include subsystem/source/function/line and port timestamp. All `BALLISTICS_DEBUG_*` and subsystem macros route through it. `BALLISTICS_DEBUG_ENABLED=0` compiles all out; Release compiles DEBUG/TRACE out.

## Port contract

The public port header declares exactly monotonic time, allocation, deallocation, debug byte output, and reserved mutex create/lock/unlock/destroy hooks. Linux implements them. Unknown platforms select `unsupported_port.c`, whose status services return `UNSUPPORTED_PLATFORM`. No file API exists.

## Thread-safety and ownership summary

Pure vector/equation functions and const model calls are reentrant. Registries, simulations, results, writers, and debug reconfiguration are not concurrently mutable. The application destroys objects in reverse order. The core never owns a file or byte-sink context.


## Exact Phase One typed configuration catalog

| Type | Fields and validation | Factory default |
|---|---|---|
| `BallisticsConstantEnvironmentConfig` | non-negative finite `air_density_kgpm3`; finite `wind_velocity_mps` | no registry factory |
| `BallisticsConstantGravityConfig` | finite `acceleration_mps2` | `[0,0,-9.80665]` for `NULL,0` |
| `BallisticsBasicDragConfig` | non-negative finite `drag_coefficient` | none; exact config required |
| `BallisticsEulerIntegratorConfig` | `maximum_state_count` in 1..64 | 64 for `NULL,0` |
| `BallisticsRk4IntegratorConfig` | `maximum_state_count` in 1..64 | 64 for `NULL,0` |
| `BallisticsGroundStopConfig` | finite `ground_height_m` | direct create only |
| `BallisticsMaximumTimeStopConfig` | positive finite `maximum_time_s` | direct create only |
| `BallisticsMaximumDistanceStopConfig` | positive finite `maximum_horizontal_distance_m` | direct create only |
| `BallisticsInvalidStateStopConfig` | reserved byte, currently ignored | direct create only |
| `BallisticsCsvWriterConfig` | copied sink descriptor with non-null `write` | none; exact config required |
| `BallisticsSimulationConfig` | positive finite step/time/horizontal distance, finite ground, non-empty/resolvable integrator ID | not factory-created |

Equation objects in Phase One are stateless: their factories accept exactly `NULL,0`. Projectile, launch, launcher metadata, projectile state, environment state, trajectory sample, stop evaluation/decision, byte sink, and the vtable/object structures shown above are ABI-visible value structures. Registry, dynamics, simulation, and result implementation structures remain opaque.

## Concrete create/destroy ownership

- Equation create functions return owned `BallisticsEquation *`; destroy with `ballistics_equation_destroy`.
- Constant environment returns an owned `BallisticsEnvironmentModel *`; destroy with `ballistics_environment_destroy`.
- Gravity and drag return owned semantic `BallisticsForceModel *`; destroy with `ballistics_force_model_destroy`.
- Euler/RK4 return owned `BallisticsIntegrator *`; destroy with `ballistics_integrator_destroy`.
- Each stop create returns owned `BallisticsStopCondition *`; destroy with `ballistics_stop_condition_destroy`.
- CSV create returns owned `BallisticsTrajectoryWriter *` which borrows the copied sink/context; destroy with `ballistics_trajectory_writer_destroy`.
- Each family registry, dynamics, simulation, and result uses its named create/destroy pair. Registries do not destroy registered or created implementations.
