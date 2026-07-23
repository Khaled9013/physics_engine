# Native GUI Architecture

## Modules

`apps/ballistics_gui/main.py` creates the `QApplication`, applies the theme, resolves the C CLI, and
owns shutdown.

`ui/main_window.py` composes the viewport and panels. It translates signals between controls,
simulation jobs, and the scene without containing physics or rendering implementation.

`simulation/` contains immutable scenario/result models, validation, CLI argument construction,
CSV parsing, and a `QRunnable` worker. It has no Panda3D dependency and can be tested headlessly.

`render/` contains coordinate conversion, procedural geometry, target scoring, and the Panda3D
range scene. It consumes trajectory samples and never evaluates drag, gravity, or an integrator.

`ui/range_widget.py` owns the native X11 child used by Panda3D. A Qt timer advances Panda3D's task
manager inside the Qt event loop. Resize and input events are forwarded explicitly.

## Rendering design

The range is generated from Panda3D primitives and programmatic meshes. This keeps the repository
self-contained and avoids third-party asset licensing. The style is a clean low-poly research range.

The first-person rig has scene nodes for the arms, hands, rifle body, barrel, stock, optic, and bolt.
Small positional and rotational animations provide idle breathing, scope raise/lower, recoil, and
reload feedback. A 2D scope mask and reticle use Panda3D's overlay scene. The reticle is decorative
and intentionally has no real optic markings.

## Shot flow

```text
mouse aim / control settings
          |
          v
immutable ScenarioConfig -- QThreadPool --> CLI + temporary CSV
          |                                      |
          |<--------- immutable ShotResult -------|
          v
trajectory scene + tracer + telemetry + target-plane scoring
```

Only the latest request identifier may update the scene. Errors return as data; workers never show
dialogs or mutate widgets.

## Performance

Panda3D owns rendering and GPU resources. Qt owns the event loop and widgets. The render timer aims
for 60 Hz and animation uses elapsed time, not frame count. The C simulation and CSV parsing run in
the Qt global thread pool. Trajectory display may decimate the line mesh while preserving full CSV.

Panda3D's threaded cull/draw pipeline is enabled only where supported. Scene mutation stays on the
application stage to avoid cross-thread graph access.

## Failure and fallback behavior

- Missing packages produce a concise setup instruction before a window is created.
- Missing CLI produces a build instruction; Python never substitutes its own physics.
- Unsupported advanced GPU features fall back to basic lighting and rendering.
- A failed shot keeps the existing scene and reports the error in the local event log.
- Window close stops timers, rejects new shots, closes Panda3D, and waits for workers.

## Test seams

Validation, CLI construction, CSV parsing, coordinate mapping, and target intersection are pure
Python. The worker accepts a CLI path. Smoke-test mode opens a deterministic scene, captures one
frame, and exits without user input.
