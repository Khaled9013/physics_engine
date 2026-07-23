# Native GUI Architecture

## Modules

`apps/ballistics_gui/main.py` creates the `QApplication`, applies the native theme, resolves the C
CLI, and owns shutdown.

`ui/main_window.py` composes the viewport, compact quick-shot controls, telemetry cards, and
collapsible advanced physics panel. It translates signals between controls, simulation jobs, and the
scene without containing physics or rendering implementation.

`simulation/` contains immutable scenario/result models, validation, CLI argument construction,
CSV parsing, and a `QRunnable` worker. It has no Panda3D dependency and can be tested headlessly.

`render/assets.py` resolves committed visual assets from the repository, validates required files,
and applies consistent texture filtering. `render/environment.py` owns the range dressing and
ground materials. `render/view_model.py` owns the first-person hand, rifle, optic, recoil, and
muzzle-effect composition. `render/range_scene.py` orchestrates those components, target feedback,
scope presentation, input, and trajectory playback.

`ui/range_widget.py` owns the native X11 child used by Panda3D. A Qt timer advances Panda3D's task
manager inside the Qt event loop. Resize and input events are forwarded explicitly.

## Rendering design

The world uses Panda3D plus `panda3d-simplepbr` for tonemapping, normal-map support, shadows, and
consistent material handling. The deterministic range layout remains code-authored, while local CC0
grass, gravel, rifle, optic, and hand assets replace the former placeholder presentation. Asset
publisher, license, checksum, conversion, and local treatment are recorded in `assets/README.md`.

The first-person rig is camera-space presentation artwork. It is isolated from world exposure so it
stays readable across sun and shadow directions. An authored hand pose hides its bundled proxy model
and carries the detailed textured rifle and separate optic. Small elapsed-time animations provide
breathing, aim-down-sights motion, recoil, flash, and recovery.

The circular scope mask is generated locally at startup. Its thin reticle, ticks, vignette, and lens
tint are decorative and intentionally have no real optic calibration. The target is a non-human
geometric practice board.

All runtime assets are committed. The GUI has no downloader, browser, socket listener, or network
fallback.

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
dialogs or mutate widgets. Rendering and recoil never alter the C result or scoring calculation.

## Performance

Panda3D owns rendering and GPU resources. Qt owns the event loop and widgets. The render timer aims
for 60 Hz and animation uses elapsed time, not frame count. The C simulation and CSV parsing run in
Qt's global thread pool. Trajectory display may decimate the line mesh while preserving full CSV.

Panda3D's threaded cull/draw pipeline is enabled only where supported. Scene mutation stays on the
application stage to avoid cross-thread graph access. Hero assets load once at renderer
initialization; no allocation or asset loading occurs per simulation step.

## Failure behavior

- Missing packages produce a concise setup instruction before a range is usable.
- Missing CLI produces a build instruction; Python never substitutes its own physics.
- A missing required hero asset fails renderer initialization with its exact local path.
- A failed shot keeps the existing scene and reports the error in the local session log.
- Window close stops timers, rejects new shots, closes Panda3D, and waits for workers.

## Test seams

Validation, CLI construction, CSV parsing, coordinate mapping, and target intersection are pure
Python. The worker accepts a CLI path. Asset tests verify required files, BAM signatures,
provenance, and the no-network runtime boundary. Smoke-test mode opens a deterministic scene, runs a
real C shot, captures a framebuffer, and exits without user input.
