# Phase 2.1 Visual Upgrade Contract

Status: accepted for implementation, 2026-07-23.

## Objective

Phase 2.1 replaces the Phase Two procedural presentation with a cohesive, asset-backed native
desktop experience. The authoritative C simulation, deterministic CSV bridge, coordinate mapping,
threading boundary, and non-networked application architecture remain unchanged.

The result should read immediately as a fictional outdoor precision-range simulator: recognisable
first-person arms hold a detailed scoped rifle, the active target is visually legible, and lighting,
materials, atmosphere, scope presentation, and shot feedback form one consistent scene.

## Scope

- Vendor redistributable CC0 models and textures under `assets/`.
- Record publisher, source URL, license, original archive checksum, conversion process, and local
  modifications for every third-party asset.
- Convert source models to deterministic GLB/BAM runtime files without requiring a converter at
  application launch.
- Replace the box-built rifle and arms with external authored models.
- Improve the procedural range with material textures, layered ground, lane furniture, target
  staging, mountains/vegetation silhouettes, shadows, haze, and a more intentional colour grade.
- Improve aim-down-sights, recoil, muzzle flash, tracer, impact feedback, and the scope overlay.
- Rebalance the PyQt layout around the viewport. Normal play controls and telemetry remain visible;
  detailed physics fields move into a clearly labelled advanced section.
- Preserve the existing local PyQt6/Panda3D application and C `QThreadPool` simulation bridge.

## Asset policy

Only assets whose publisher page and included license permit redistribution are vendored. CC0 is
preferred. Asset acquisition is a development-time operation; the installed application performs
no network requests and starts no listener.

Runtime code must resolve assets relative to the repository/package rather than the current working
directory. A missing optional decorative asset may fall back to a local procedural substitute. A
missing rifle or arms asset is a renderer initialization error, because silently returning to the
placeholder rig would hide a broken installation.

Third-party source archives and unrelated engine projects are not committed. Only the source files
needed for provenance and conversion, optimized runtime files, textures, and license notices are
kept.

## Boundaries

- No real weapon profile, optic calibration, live-fire input, automatic aiming, or human target.
- No browser, HTTP service, socket listener, analytics, or runtime downloader.
- No Python, Panda3D, Qt, model, texture, or filesystem dependency enters `ballistics_core`.
- Render animations never alter the numerical trajectory or scoring calculation.
- The visual model is presentation artwork and is not a dimensional representation used by the
  solver.

## Acceptance criteria

- The hip view visibly contains connected first-person arms and a detailed scoped rifle.
- The active geometric practice target is readable in hip view and centred in scope view.
- The range has visible depth cues, lighting, shadows, lane structure, environmental dressing, and
  no large untextured placeholder blocks in the first-person rig.
- Scope transition is smooth and the circular view includes a legible reticle, vignette, and range
  status without claiming real optic calibration.
- A shot shows recoil, flash, tracer, target-plane feedback, and an impact marker while the GUI
  remains responsive.
- The normal play surface presents Fire, Scope, Reset, target distance, wind, integrator, and core
  telemetry without requiring the user to scan the complete physics form.
- All prior C and Python tests pass.
- A native GPU smoke test captures and inspects hip, scoped, and post-shot frames.
- `rg` confirms the GUI contains no network client/server implementation.
