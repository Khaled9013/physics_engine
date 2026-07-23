# Adding an Equation

Equations are pure reusable calculations. They do not represent accumulated physical forces and do not know the simulation loop. Put trajectory solvers, aiming calculations, and future tracking calculations in the equation system. Put gravity, drag, gust, or precipitation contributions in force models.

## Minimal process

1. Add `include/ballistics/equations/temperature_ratio_equation.h`.
2. Add `src/equations/temperature_ratio_equation.c`.
3. Implement a typed function plus the common metadata/vtable adapter.
4. Add one identifier/factory entry in `ballistics_register_builtin_equations`.
5. Add a unit test for typed and generic paths.
6. Add the `.c` file to `ballistics_core` in CMake.

## Small complete example

Public header:

```c
#ifndef BALLISTICS_EQUATIONS_TEMPERATURE_RATIO_EQUATION_H
#define BALLISTICS_EQUATIONS_TEMPERATURE_RATIO_EQUATION_H
#include "ballistics/interfaces/equation_interface.h"
#include <stddef.h>
#define BALLISTICS_TEMPERATURE_RATIO_ID "temperature-ratio.v1"
typedef struct { double temperature_k; double reference_k; } TemperatureRatioInput;
BallisticsStatus ballistics_temperature_ratio_evaluate(
    const TemperatureRatioInput *input, double *out_ratio);
BallisticsStatus ballistics_temperature_ratio_equation_create(BallisticsEquation **out);
BallisticsStatus ballistics_temperature_ratio_equation_factory(
    const void *config, size_t config_size, BallisticsEquation **out);
#endif
```

Core typed calculation:

```c
#include <math.h>
BallisticsStatus ballistics_temperature_ratio_evaluate(
    const TemperatureRatioInput *input, double *out_ratio)
{
    if (input == NULL || out_ratio == NULL || !isfinite(input->temperature_k) ||
        !isfinite(input->reference_k) || input->temperature_k <= 0.0 ||
        input->reference_k <= 0.0) {
        return BALLISTICS_STATUS_INVALID_ARGUMENT;
    }
    *out_ratio = input->temperature_k / input->reference_k;
    return isfinite(*out_ratio) ? BALLISTICS_STATUS_OK : BALLISTICS_STATUS_NUMERICAL_ERROR;
}
```

The source also allocates a `BallisticsEquation`, fills immutable identifier/name/category/input/output metadata, supplies a generic adapter that requires exactly `sizeof(TemperatureRatioInput)` and `sizeof(double)`, and frees through the port in `destroy`. Stateless creation accepts no config; its factory accepts only `NULL,0`.

Registration is one line in the built-in table:

```c
{BALLISTICS_TEMPERATURE_RATIO_ID, ballistics_temperature_ratio_equation_factory},
```

No simulation, dynamics, force-model, or integrator file changes.
