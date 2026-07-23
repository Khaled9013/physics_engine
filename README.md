# Ballistics Simulation

A modular C17 virtual research simulator for three-dimensional point-mass projectile motion.
Phase One provides the simulation library and CLI; Phase Two adds a native Linux visualization
using PyQt6 and Panda3D.

Coordinates are `+x` downrange, `+y` right, and `+z` upward. Core calculations use SI `double`.

## Capabilities

Phase One provides configurable projectile/launch data, constant gravity/density/wind, quadratic
vector drag, Euler and RK4 integration, interpolated ground impact, deterministic CSV output,
registries, debug logging, and CTest/Unity verification.

Phase Two and the Phase 2.1 visual upgrade provide a local desktop window with:

- An asset-backed outdoor practice range with local CC0 ground materials and depth dressing.
- An authored first-person hand pose carrying a textured fictional precision rifle and optic.
- Panda3D PBR tonemapping, shadows, smooth scope presentation, recoil, flash, tracer, impact, and
  target reaction.
- A focused quick-shot surface with collapsible advanced solver, projectile, and environment fields.
- Live time, range, drift, speed, hit/miss, and optional session telemetry.
- `QThreadPool` shot calculation so the UI and renderer remain responsive.

The desktop application contains no browser, web view, HTTP server, or network listener. It is a
fictional visualization, not a real-firearm trainer: it ships no real weapon profiles, optic
calibration, human targets, live hardware integration, or automatic aiming solver.

## Core requirements

- Linux
- CMake 3.20 or newer
- GCC or Clang with C17 support
- Standard C library and `libm`

Unity is vendored. The core has no Python, Qt, Panda3D, GUI, or filesystem dependency.

## Build and test the C project

```bash
./scripts/build_debug.sh
./scripts/run_tests.sh
```

Equivalent manual commands:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBALLISTICS_BUILD_TESTS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Release uses `-O2` while preserving IEEE-754 semantics:

```bash
./scripts/build_release.sh
```

Shared library:

```bash
cmake -S . -B build-shared -DBALLISTICS_BUILD_SHARED=ON
cmake --build build-shared
```

Unsafe floating-point flags such as `-ffast-math`, `-Ofast`, and `-ffinite-math-only` are rejected.

## Run the native GUI

The desktop port requires Linux/X11 or XWayland, Python 3.10+, working OpenGL drivers, and the
pinned PyQt6, Panda3D, and panda3d-simplepbr packages.
Prepare an isolated project environment once:

```bash
./scripts/setup_gui.sh
```

Launch the local application:

```bash
./scripts/run_gui.sh
```

Controls:

- Click the 3D viewport to capture the mouse; press `Esc` to release it.
- Move the mouse to aim.
- Left-click or select **FIRE SHOT** to run and animate a C-engine shot.
- Right-click or select **Toggle optic** to raise/lower the decorative scope.
- Select **Reset range** to clear the trajectory and target feedback.

The launch script builds the C CLI if it is missing. It uses `.venv/` only and defaults Qt to the
`xcb` backend required by the embedded Panda3D X11 child window.

## Test the GUI

```bash
./scripts/run_gui_tests.sh
```

On a machine with an active X11 display, include a real GPU/window smoke test:

```bash
./scripts/run_gui_tests.sh --smoke
```

The smoke run executes a real C shot, renders the scope view, saves a temporary framebuffer, and
exits automatically.

## CLI

The original command remains supported:

```bash
./build/apps/ballistic_cli/ballistics_cli --integrator rk4.v1 --time-step 0.002 --max-time 5.0 --output trajectory.csv --debug-level warning
```

Run `./build/apps/ballistic_cli/ballistics_cli --help` for the expanded synthetic scenario options.
The CLI owns its `FILE *` and supplies a byte sink; the core and CSV writer never open files.

## Threading and GPU boundary

Qt widgets and Panda3D scene nodes stay on the GUI thread. Immutable shot jobs invoke the C CLI and
parse deterministic CSV in Qt's worker pool. Panda3D owns OpenGL rendering and may use a threaded
cull/draw pipeline. Simulation workers never mutate the scene graph.

Ballistics coordinates map to Panda3D exactly once: `(x, y, z)` becomes `(y, x, z)` because Panda3D
uses `+y` as forward.

## Repository overview

- `include/ballistics/`: public C API and extension interfaces
- `src/`: core, equations, physics, integrators, output, registries, and Linux port
- `apps/ballistic_cli/`: Linux CLI and file-backed byte sink
- `apps/ballistics_gui/`: PyQt shell, worker bridge, and modular Panda3D renderer
- `assets/`: committed CC0 presentation assets and provenance
- `tests/`: Unity C tests and Python GUI tests
- `docs/phase_two_plan.md`: native GUI scope and acceptance contract
- `docs/gui_architecture.md`: desktop module, rendering, and thread boundaries
- `scripts/`: build, sanitizer, GUI setup, launch, and test helpers

## Extension points

Stable registries resolve equations, force models, integrators, and writers by versioned identifier.
See `docs/adding_an_equation.md`, `docs/adding_a_force_model.md`,
`docs/adding_an_integrator.md`, and `docs/porting_guide.md`.

## Licensing note

Repository source is MIT licensed. Third-party CC0 asset provenance is recorded in
`assets/README.md`. Panda3D is BSD licensed. PyQt6 is available under GPL-3.0 or a
commercial license; distributing a combined GUI build with the GPL edition requires compliance with
its GPL terms. Organizations needing different distribution terms must obtain the appropriate PyQt
commercial license.

## Known GUI limitations

The embedding port uses Linux/X11 or XWayland, not native Wayland. The upgraded range remains
stylized rather than photoreal, and the Python application currently communicates with C through the
CLI rather than direct shared-library bindings. See `docs/phase_2_1_acceptance.md` for the verified
visual-upgrade matrix.
