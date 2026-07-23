# Phase Two Native Range Plan

Status: accepted for implementation, 2026-07-23.

## Objective

Phase Two adds a local Linux desktop application around the completed Phase One simulator. It uses
PyQt6 for the application shell and Panda3D for an embedded, GPU-rendered practice range. There is
no browser, HTTP server, web view, or network service.

The experience is a fictional first-person target range: stylized arms hold a fictional precision
rifle, the user can look around, enter a decorative scope view, fire simulated shots, and inspect
the resulting trajectory and impact telemetry.

## Scope

- A native PyQt6 main window with simulation controls, telemetry, event log, and 3D viewport.
- A Panda3D viewport embedded into the Qt window on the supported Linux/X11 host.
- A procedural practice range with ground, sky, firing position, berms, and non-human steel/paper
  targets.
- Procedural first-person arms and a fictional precision-rifle silhouette with a scope.
- Mouse aiming, scope toggle, fire/reload animation, target reset, and camera recentering.
- GPU rendering through Panda3D, including lighting, depth, and antialiasing.
- Background simulation and CSV parsing through `QThreadPool` so the event loop does not block.
- Optional Panda3D cull/draw pipeline threading when the renderer supports it.
- Controls for the existing synthetic projectile, launch, environment, integrator, and time step.
- Trajectory trace, impact marker, hit/miss scoring, and time/range/velocity telemetry.

## Boundaries and non-goals

The application is a fictional visualization, not a real-firearm trainer. Phase Two does not ship
real weapon names or profiles, real optic calibration, human targets, live sensor or weapon input,
an inverse aiming solver, or instructions for operating a physical weapon. It also does not add
terrain simulation, 6-DOF dynamics, multiplayer, networking, or downloadable plugins.

The Phase One `ballistics_core` public API remains frozen. No Python, Qt, Panda3D, rendering, input,
or filesystem dependency is added to the C core.

## Dependency flow

```text
PyQt desktop shell
    |-- control panels and telemetry
    |-- QThreadPool simulation jobs --> ballistics_cli --> ballistics_core
    `-- embedded Panda3D viewport --> GPU
```

The C CLI remains the process boundary in this milestone. The GUI creates an application-owned
temporary CSV, invokes the CLI with an argument list and no shell, parses the result, then removes
the temporary file. A future direct binding can replace this bridge without changing the scene.

## Coordinate mapping

Ballistics coordinates are `+x` downrange, `+y` right, `+z` up. Panda3D coordinates are mapped as:

```text
panda_x = ballistics_y
panda_y = ballistics_x
panda_z = ballistics_z
```

The mapping is implemented once and unit-tested. Rendering modules do not reinterpret units.

## Threading contract

- Qt widgets, Panda3D scene nodes, and input state are mutated only on the GUI thread.
- Each shot calculation is an immutable job executed by `QThreadPool`.
- Workers invoke the CLI, parse CSV, and emit an immutable result through Qt signals.
- A request identifier prevents an old result replacing a newer shot.
- Closing the application rejects new work and waits briefly for active jobs.
- Panda3D may split cull/draw work internally; workers never access its scene graph.

## Visual interaction contract

- Mouse movement changes yaw/elevation within bounded range limits.
- Right mouse or **Scope** toggles magnification and the scope overlay.
- Left mouse or **Fire** requests one simulation when no shot job is active.
- The rendered projectile and tracer follow samples returned by the C engine.
- Scoring interpolates the trajectory at a target plane and compares the rendered target radius;
  it does not calculate an aim correction.
- Targets are steel plates or printed geometric boards. No human character is a target.

## Technology

- Python 3.10 or newer; the initial host has Python 3.14.
- PyQt6 6.11.x.
- Panda3D 1.10.16.
- No browser framework, JavaScript runtime, web server, NumPy, or external model asset.

PyQt6 is GPL-3.0/commercial dual licensed. Panda3D is BSD licensed.

## Stages

1. Contract: this plan, GUI architecture, ADR, and resume pointer.
2. CLI controls: validated synthetic scenario parameters while preserving old defaults.
3. Desktop shell: PyQt window, control model, telemetry, worker, and shutdown rules.
4. Range renderer: embedded Panda3D scene, procedural assets, input, scope, and animation.
5. Tests and packaging: pinned dependencies, setup/run scripts, Python tests, README.
6. Acceptance: CTest, sanitizers, Python tests, real-CLI shot, GPU/window smoke test, clean build,
   and repository push.

## Acceptance criteria

- `./scripts/setup_gui.sh` prepares an isolated environment.
- `./scripts/run_gui.sh` opens a native desktop window and starts no network listener.
- A shot never blocks window movement, controls, or frame updates.
- The viewport shows a first-person fictional rifle, scope overlay, practice targets, shot animation,
  trajectory, and impact feedback.
- Identical GUI settings produce the same CSV bytes through the same CLI binary.
- Existing Phase One tests and sanitizers still pass.
- Python tests and a Linux screenshot smoke test pass.
