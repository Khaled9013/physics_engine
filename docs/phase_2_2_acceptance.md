# Phase 2.2 Visual Realism Acceptance

Date: 2026-07-26.

## Verified outcome

Phase 2.2 replaces the flat Phase 2.1 presentation with an overcast outdoor scene while preserving
the Phase One C contracts and the Phase Two thread boundary.

- The sky is a camera-following dome carrying a committed CC0 equirectangular panorama with real
  cloud structure. Cube faces projected from the same panorama drive `simplepbr`'s image-based
  lighting, so the scene is lit by the sky rather than by a flat ambient term.
- Exponential haze now renders. It separates the near lane, the backstop, and the distant ridges
  into visible depth layers.
- Cast shadows are visible on the ground from the bay beam, posts, bench, and vegetation.
- Ground and lane are generated meshes with tangents, world-space texture coordinates, and gentle
  relief that stays exactly flat over the lane corridor and firing position. They carry full
  base/normal/ORM texture sets.
- The rifle renders with its committed normal and ORM maps bound, showing wood grain and metal
  detail that were previously absent.
- The first-person rig is lit rather than unshaded, seats its optic on the receiver, and fits the
  bundled arms to the detailed rifle instead of to the proxy weapon they were authored around.
- The target is a printed face on flat geometry, legible in the hip view and centred under the
  optic, with accumulating impact decals.
- Ground carries large-scale colour variation written per vertex, and the lane carries compacted
  edges and a worn centre, so neither reads as a single tiled stamp.
- Tree canopies are grown along branch lines with real gaps rather than drawn as solid masses, and
  objects sit on soft contact-shadow patches so they no longer appear to hover.
- Escape opens a settings panel carrying display mode, window size, and an aim-speed slider.
- Every runtime model and texture is committed. Publisher, license, source URL, checksum,
  conversion, and local treatment are recorded in `assets/README.md`.
- The GUI performs no runtime network access.

## Defects corrected

Four faults in the Phase 2.1 renderer were found and fixed. Each independently flattened the image:

- `simplepbr` was built with `enable_fog=False` while the scene called `render.setFog`. Because
  `simplepbr` applies fog inside its shader, the haze was never rendered at all.
- The sun's shadow lens was never sized. Panda3D's `DirectionalLight` defaults to an orthographic
  film of roughly a unit square, so on a kilometre-long range effectively nothing was shadowed.
- `add_box` treated `position` as a centre while Panda3D's `models/box` spans `(0,0,0)`–`(1,1,1)`,
  displacing every box in the range by its own half-extent. This is what placed an 11 m firing pad
  across the lower third of the hip view.
- The rifle's committed normal and ORM maps were resolved and exposed but never bound.

A fifth fault was introduced and fixed during this phase: the automated smoke test scheduled its
capture on a fixed delay from launch, and the richer scene takes longer to build than that delay
allowed, so it captured blank frames. The sequence is now chained off the renderer's ready signal,
with a timeout that fails loudly instead of capturing nothing.

## Settings panel

Escape raises a modal panel with display mode, window size, and a 0-100 aim-speed slider. Every
control applies immediately, because aim speed can only be judged by moving the view.

The previous fixed hip speed of 0.048 degrees per mouse count was reported as too fast. Aim speed
is now user-controlled across 0.0035 to 0.150 degrees per count, mapped geometrically so each step
changes speed by a constant proportion. The default of 35 resolves to 0.0130 degrees per count,
roughly a quarter of the previous speed. Settings apply for the session and are not persisted.

## Scenario defects corrected

Reported after play-testing and fixed in stage 07:

- The Headwind field mapped straight to the solver's downrange wind component with no sign
  change, so a positive headwind blew downrange and lengthened the shot. With the original
  0.35 kg test projectile, +6 m/s of "headwind" reached 2052.9 m against 2047.8 m in still
  air. It now shortens the shot to 2042.6 m.
- Diameter was validated and stored but never entered any force calculation; only reference
  area did. Changing diameter from 9 mm to 50 mm produced byte-identical results. Diameter now
  drives reference area as pi*d^2/4, and the area remains editable as an explicit override.
- Mass and diameter spin boxes carried four decimals and reference area eight, which silently
  rounded preset values and reintroduced the same diameter/area mismatch. They now carry five,
  five, and ten, and every preset round-trips to within 1.3e-6 relative.
- Every spin box stepped by Qt's default of 1.0, meaningless for a mass in kilograms or an
  area in square metres. Step size is now derived from each field's declared precision.

The solver itself was checked and found correct: the C output was reproduced exactly by an
independently written RK4 integrator, converged between 1e-4 and 1e-5 steps.

## Optic zeroing

Reported after play-testing: the shot did not line up with the centre of the reticle. Measured
with the crosshair placed exactly on the target centre, the impact fell 0.612 m low at 100 m and
1.305 m low at 150 m, with lateral error of exactly 0.000 m. The cause was that nothing zeroed the
optic — the reticle marked the bore axis, so the entire bullet drop appeared as error.

The optic is now zeroed at a user-set distance, defaulting to 150 m. `simulation/zeroing.py`
searches for the launch elevation whose trajectory crosses the line of sight at that distance,
driving the same C solver rather than integrating in Python, and applies the resulting angle to
every aim. Verified impact relative to the crosshair, still air:

| Zero | Offset | Probes | Impact at zero distance |
| --- | --- | --- | --- |
| 100 m | +6.12 mil | 2 | -0.0 mm |
| 150 m | +8.69 mil | 2 | -0.1 mm |
| 200 m | +11.52 mil | 3 | -0.2 mm |
| 300 m | +17.69 mil | 4 | -0.6 mm |

Away from the zero the trajectory behaves as a zeroed rifle should: with a 150 m zero the shot
lands 0.22 m high at 50 m and 0.26 m high at 100 m, and 0.57 m low at 200 m. Flatter loads need
less elevation — the .308 Match preset needs 1.81 mil at 150 m against 8.69 mil for the slow
default.

Two solver defects were found and fixed while building this. The initial probe was fired level,
which cannot reach any zero beyond about 150 m from shoulder height, so the solver abandoned zeros
that plainly existed at a higher angle; the trial angle is now escalated until the shot carries.
And solving at the finest allowed time step cost 219 ms before the first shot; probes are now
capped at a 1 ms step, which yields a bit-identical angle in 29 ms.

## Projectile presets

`apps/ballistics_gui/data/projectiles.json` holds eleven presets across rifle, pistol, rimfire,
and sphere categories. Selecting one fills mass, diameter, reference area, drag coefficient, and
muzzle speed; editing any of those by hand returns the selector to Custom.

Mass and diameter are published nominal figures. Because the solver applies a constant drag
coefficient, each coefficient is fitted so the trajectory reproduces a published remaining velocity
at a stated reference distance. Both figures are stored, and a test re-derives every coefficient
and asserts it still matches within 1%; the worst current error is 0.33%. A malformed or missing
preset file degrades to custom-only with the reason shown rather than blocking the application.

## Verified host

- Linux/X11 using Qt's `xcb` backend and Panda3D's `glxGraphicsPipe`.
- GCC 13.3.0.
- Python 3.14.3.
- PyQt6 / Qt 6.11.0.
- Panda3D 1.10.16.
- panda3d-simplepbr 0.13.1.
- NVIDIA GeForce RTX 3050, driver 595.71.05, OpenGL 4.6.0.

## Test results

- Python GUI, bridge, worker, coordinate, scoring, asset, layout, terrain, settings, projectile
  preset, controls, and zeroing tests: 89/89 passed, up from 17 in Phase 2.1. New coverage includes layout
  and terrain relief, the sky cube-face pattern, aim speed, preset loading and schema validation,
  the headwind sign, and the diameter/area coupling.
- CTest unit, integration, and CLI-option targets: 3/3 passed.
- ASan/UBSan clean rebuild in `build-sanitizers/`: 3/3 CTest targets passed with no sanitizer
  report.
- Fresh Debug configure/build in `build-phase22-clean/`: passed with strict warnings and no
  warning emitted.
- Native GPU smoke through `./scripts/run_gui_tests.sh --smoke`: passed.
- Hip, scoped, post-shot, and full PyQt window captures were inspected. An elevated debug view was
  additionally used to confirm shadow casting and ground continuity.
- `grep` over `apps/ballistics_gui/` found no HTTP, socket, urllib, requests, httpx, websocket,
  listen, or bind usage.

## Preserved boundaries

`ballistics_core` was not changed by Phase 2.2. It still has no Python, Qt, Panda3D, game-engine,
asset, filesystem, or network dependency. Models remain presentation artwork and provide no
dimension to the solver. The target remains non-human, and the application includes no automatic
aiming, live hardware integration, or real optic calibration. The optic overlay is decorative.

## Known limitations

- Native embedding still targets X11/XWayland; a native Wayland child-window path is not
  implemented.
- The bundled CC0 arms are coarse low-polygon geometry. They are fitted, lit, and kept dark at the
  frame edge so they read as sleeves rather than flat slabs, but they remain the weakest element of
  the rig. Replacing that asset is the single highest-value further improvement.
- Trees are cross-billboard cut-outs with procedurally generated foliage, and distant ridges are
  tinted ellipsoids. Both rely on haze and distance to read correctly and do not stand up to close
  inspection.
- `simplepbr` exposes one texture coordinate set, so a single mesh cannot blend two ground
  materials. Repetition is broken by splitting the near and far field into separate meshes with
  different textures and tiling, and the seam is hidden by haze rather than blended.
- Scene construction takes roughly 2.5 s on the acceptance host. It is not incremental and shows no
  progress indication.
- There is still no audio, terrain streaming, weather volume, saved scenario library, 6-DOF model,
  post-process bloom or depth of field, or multiplayer.
- The score remains visual target-plane interpolation, not an inverse aiming solution.
