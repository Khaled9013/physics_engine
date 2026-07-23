# Ballistics Simulation — Phase One

A modular C17 virtual research simulator for three-dimensional point-mass projectile motion. It is entirely software-defined: no physical weapon, live sensor, or live-fire hardware is used or required.

Phase One provides configurable projectile/launch data, constant gravity/density/wind, vector quadratic drag, replaceable Euler and RK4 integration, interpolated ground impact, time/distance/numerical stop conditions, deterministic CSV output, a Linux CLI, registries, centralized debug logging, and Unity/CTest verification.

Coordinates are `+x` downrange, `+y` right, `+z` upward. All internal values are SI `double`.

## Requirements

- Linux
- CMake 3.20 or newer
- GCC with C17 support (Clang is also supported)
- A normal C standard library and `libm`

Unity is vendored; no system test framework is needed. The core has no Python, GUI, game-engine, or filesystem dependency.

## Build and test

```bash
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Debug \
  -DBALLISTICS_BUILD_TESTS=ON \
  -DBALLISTICS_ENABLE_SANITIZERS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Release uses `-O2` while preserving IEEE-754 semantics:

```bash
cmake -S . -B build-release \
  -DCMAKE_BUILD_TYPE=Release \
  -DBALLISTICS_BUILD_TESTS=OFF
cmake --build build-release
```

Shared library:

```bash
cmake -S . -B build-shared -DBALLISTICS_BUILD_SHARED=ON
cmake --build build-shared
```

Coverage is enabled with `-DBALLISTICS_ENABLE_COVERAGE=ON`. Unsafe flags including `-ffast-math`, `-Ofast`, and `-ffinite-math-only` are rejected at configure time.

Convenience scripts live in `scripts/`.

## CLI

```bash
./build/apps/ballistic_cli/ballistics_cli \
  --integrator rk4.v1 \
  --time-step 0.001 \
  --max-time 10.0 \
  --output trajectory.csv \
  --debug-level info
```

The CLI uses a clearly fictional synthetic profile. It owns the `FILE *` and supplies a byte sink; the core and CSV writer never open or close files.

## Repository overview

- `include/ballistics/`: public C API and extension interfaces
- `src/equations/`: reusable pure calculations
- `src/physics/`: environment, force models, and dynamics aggregation
- `src/integrators/`: fixed-step Euler and classical RK4
- `src/stop_conditions/`: concrete termination rules
- `src/output/`: byte-sink CSV serialization
- `src/port/`: exactly one selected platform implementation
- `apps/`: application-owned resources and CLI
- `tests/`: vendored Unity, unit tests, integration tests, memory sink
- `docs/`: contracts, ADR, architecture, extension and porting guides

## Extension points

Stable registries resolve equations, force models, integrators, and writers by versioned identifier. Typed create functions remain available without registries. See:

- `docs/adding_an_equation.md`
- `docs/adding_a_force_model.md`
- `docs/adding_an_integrator.md`
- `docs/porting_guide.md`

Phase One intentionally does not implement 6-DOF, adaptive stepping, terrain, sensors, inverse aiming, tracking, Monte Carlo analysis, GUI/game-engine integration, JSON profiles, dynamic-library plugins, or Python bindings. `docs/python_integration_plan.md` describes the prepared ABI path.
