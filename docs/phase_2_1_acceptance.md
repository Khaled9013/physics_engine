# Phase 2.1 Visual Upgrade Acceptance

Date: 2026-07-23.

## Verified outcome

Phase 2.1 replaces the Phase Two placeholder presentation while preserving the Phase One C
contracts and Phase Two thread boundary.

- The range uses local CC0 grass and gravel textures, deterministic range dressing, lighting,
  shadows, distant depth cues, and a geometric practice target.
- The camera-space rig combines an authored first-person hand pose, a separately textured precision
  rifle, and a separate optic. The bundled coarse proxy model is not rendered.
- Hip and optic views provide breathing motion, smooth aim-down-sights blending, recoil, muzzle
  flash, tracer playback, impact feedback, and target reaction.
- The PyQt panel keeps integrator, target, launch speed, elevation, azimuth, crosswind, telemetry,
  Fire, Optic, and Reset on the normal play surface. Detailed solver, projectile, and environment
  values are collapsible.
- Every runtime model and texture is committed. Publisher, license, source URL, checksum, conversion,
  and local treatment are recorded in `assets/README.md`.
- The GUI performs no runtime network access.

## Verified host

- Linux/X11 using Qt's `xcb` backend and Panda3D's `glxGraphicsPipe`.
- GCC 13.3.0.
- Python 3.14.3.
- PyQt6 / Qt 6.11.0.
- Panda3D 1.10.16.
- panda3d-simplepbr 0.13.1.
- NVIDIA GeForce RTX 3050 acceptance host, driver 595.71.05.

## Test results

- Python GUI, bridge, worker, coordinate, scoring, and asset tests: 17/17 passed.
- CTest unit, integration, and CLI-option targets: 3/3 passed.
- ASan/UBSan clean rebuild: 3/3 CTest targets passed with no sanitizer report.
- Fresh Debug configure/build in `build-phase21-clean/`: passed with strict warnings and no warning.
- Native GPU smoke through `./scripts/run_gui_tests.sh --smoke`: passed.
- Hip, circular scope, post-shot viewport, and full PyQt window captures were inspected.
- Two identical GUI bridge configurations remain byte-identical at the authoritative CSV boundary.
- Source search and automated tests found no HTTP or socket implementation in
  `apps/ballistics_gui/`.

## Preserved boundaries

`ballistics_core` was not changed by Phase 2.1. It still has no Python, Qt, Panda3D, game-engine,
asset, filesystem, or network dependency. Models are presentation artwork only and do not provide
physical dimensions to the solver. The target remains non-human and the application includes no
automatic aiming, live hardware integration, or real optic calibration.

## Known limitations

- Native embedding targets X11/XWayland; a native Wayland child-window path is not implemented.
- The range and first-person pose are stylized game art, not photoreal scanned terrain or a
  full-body character simulation.
- Python still invokes the C CLI in a worker process; direct shared-library bindings remain future
  work.
- The score is visual target-plane interpolation, not an inverse aiming solution.
- There is no audio, terrain streaming, weather volume, saved scenario library, 6-DOF model, or
  multiplayer.
