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
and applies consistent texture filtering and colour-space handling.

`render/range_layout.py` holds every placement, dimension, and tint as plain data. It imports
nothing from Panda3D, so the layout is inspected by headless tests. `render/terrain.py` generates
ground meshes with tangents, world-space texture coordinates, and deterministic relief.
`render/materials.py` builds physically-based materials from committed texture sets.
`render/sky.py` owns the sky dome and distance haze. `render/lighting.py` owns the sun, bounce
fill, ambient floor, and shadow framing.

`render/environment.py` is a builder over those parts and holds no content of its own.
`render/view_model.py` owns the first-person hand, rifle, optic, recoil, and muzzle-effect
composition. `render/range_scene.py` orchestrates those components, target feedback, scope
presentation, input, and trajectory playback.

`ui/range_widget.py` owns the native X11 child used by Panda3D. A Qt timer advances Panda3D's task
manager inside the Qt event loop. Resize and input events are forwarded explicitly.

`ui/settings_dialog.py` is the escape-key panel: display mode, window size, and aim speed. It holds
no state of its own and emits a signal per control; `MainWindow` owns the values and applies them.
Escape reaches it by two routes, because Panda3D only receives the key while it holds the pointer —
the scene forwards its own escape binding, and a window-level shortcut covers every other case.
Both release aim first, so the pointer is usable when the panel appears.

Aim speed is exposed as a 0-100 setting rather than as degrees per mouse count, and maps onto that
range geometrically so each step changes speed by a constant proportion. A linear map would crowd
every usable slow value into the bottom few steps. The optic keeps a fixed fraction of the hip
speed, because the same hand movement must cover far less angle through a magnified view.

## Rendering design

The world uses Panda3D plus `panda3d-simplepbr` for tonemapping, normal-map support, shadows, and
consistent material handling. The deterministic range layout remains code-authored, while local CC0
grass, gravel, sky, rifle, optic, and hand assets replace the former placeholder presentation. Asset
publisher, license, checksum, conversion, and local treatment are recorded in `assets/README.md`.

Lighting is dominated by the sky rather than by the sun: cube faces projected from the committed
panorama drive `simplepbr`'s image-based lighting, and the directional sun contributes shaping and
the only shadow-casting frustum. That frustum is sized and re-centred explicitly, because Panda3D's
default shadow film covers roughly a unit square and would otherwise shadow nothing on a
kilometre-long range. Distance haze uses `simplepbr`'s in-shader exponential fog, which requires the
pipeline to be built with fog enabled — `render.setFog` alone has no effect under its shader.

`simplepbr` selects samplers by texture-stage mode: `MModulate` for base colour, `MNormal` for
normals, `MSelector` for packed occlusion/roughness/metalness. It multiplies sampled roughness and
metalness by the `Material` scalars, so ORM-backed materials keep those scalars at 1.0.

The first-person rig is camera-space presentation artwork, lit by scene lights plus its own key and
rim so it stays readable across sun directions without being unshaded. It is excluded from shadow
casting, because presentation-scale artwork would cast a wrongly sized shadow onto the ground. The
bundled arms are fitted to the detailed rifle's bounding box rather than to the coarse proxy weapon
they were authored around. Small elapsed-time animations provide breathing, aim-down-sights motion,
recoil, flash, and recovery.

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
Python. Range layout and terrain relief are too: `range_layout` and `terrain_height` are covered by
tests that need no graphics context, which is why layout data lives apart from the builder that
consumes it. Asset tests walk the nested asset record recursively, so a newly vendored file is
covered without editing the test, and they verify BAM signatures, provenance, and the no-network
runtime boundary.

Smoke-test mode opens a deterministic scene, runs a real C shot, captures a framebuffer, and exits
without user input. Its sequence is chained off the renderer's ready signal rather than off fixed
delays from launch: scene construction cost varies with the host and with range content, and a
wall-clock schedule silently captures blank frames whenever loading runs long.
