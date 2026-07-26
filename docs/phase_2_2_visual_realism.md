# Phase 2.2 Visual Realism Contract

Status: implemented and verified, 2026-07-26. See [`phase_2_2_acceptance.md`](phase_2_2_acceptance.md).

## Objective

Phase 2.1 produced an asset-backed range, but it read as flat artwork: a single-colour sky,
untextured-looking ground, no visible shadows, and a first-person rig rendered without shading.
Phase 2.2 raises the presentation to a grounded, overcast outdoor scene while leaving the
authoritative C simulation, the deterministic CSV bridge, the coordinate mapping, the threading
boundary, and the non-networked architecture unchanged.

The art direction is soft overcast light. Diffuse sky illumination, aerial perspective, and gentle
relief carry the realism rather than hard sunlight, which is also the direction most forgiving of
the simple geometry the range is built from.

## Defects corrected

Phase 2.2 fixes four concrete faults in the Phase 2.1 renderer, each of which independently
flattened the image:

- `simplepbr` was constructed with `enable_fog=False` while the scene called `render.setFog`.
  `simplepbr` applies fog inside its shader, so the atmospheric haze was never rendered and the
  range had no distance cue at all.
- Nothing sized the sun's shadow lens. Panda3D's `DirectionalLight` ships an orthographic shadow
  film covering roughly a unit square, so on a kilometre-long range the shadow map covered a
  negligible sliver and no surface visibly received a shadow.
- `add_box` treated its `position` argument as a centre, but Panda3D's `models/box` spans
  `(0,0,0)`–`(1,1,1)`. Every box in the range was therefore displaced by its own half-extent,
  which is what placed an 11 m firing pad across the lower third of the hip view.
- The rifle's committed normal and ORM maps were resolved and exposed but never bound; only the
  base colour was applied, leaving the weapon without surface detail.

## Scope

- Replace the flat background colour with a camera-following sky dome carrying a committed
  equirectangular panorama, and drive image-based lighting from cube faces projected from it.
- Rebuild the ground as generated meshes carrying tangents, world-space texture coordinates, and
  deterministic relief that stays flat over the lane corridor and firing position.
- Bind full base/normal/ORM texture sets for ground, lane, and rifle.
- Frame the sun's shadow volume on the played area and exclude the camera-space rig from casting.
- Light the first-person rig instead of disabling shading on it, and fit the bundled arms to the
  detailed rifle rather than to the coarse proxy weapon they were authored around.
- Replace the target's stack of flattened spheres with a printed face on flat geometry, and add
  accumulating impact decals.
- Separate range content from range construction so the layout can be tested without a GPU.

## Architecture

`render/range_layout.py` holds all placement and tint data as plain values and imports nothing
from Panda3D, so it is inspected by headless tests. `render/terrain.py` generates ground meshes.
`render/materials.py` builds physically-based materials from committed texture sets.
`render/sky.py` owns the dome and haze. `render/lighting.py` owns the sun, fill, ambient floor,
and shadow framing. `render/environment.py` is reduced to a builder over those parts, and
`render/range_scene.py` to orchestration.

## Material convention

`simplepbr` selects shader samplers by texture-stage mode, not by stage name: `MModulate` supplies
base colour, `MNormal` the tangent-space normal map, and `MSelector` the packed
occlusion/roughness/metalness map. Its shader multiplies the sampled roughness and metalness by
the corresponding `Material` scalars, so any material backed by an ORM map leaves those scalars at
1.0 — otherwise the committed map is silently attenuated. Base-colour maps are loaded as sRGB;
normal and ORM maps carry measurements and stay in their stored encoding.

## Boundaries

Unchanged from Phase 2.1 and restated because this phase touches presentation only:

- No real weapon profile, optic calibration, live-fire input, automatic aiming, or human target.
- No browser, HTTP service, socket listener, analytics, or runtime downloader.
- No Python, Panda3D, Qt, model, texture, or filesystem dependency enters `ballistics_core`.
- Render animations never alter the numerical trajectory or the scoring calculation.
- The visual model is presentation artwork and supplies no dimension to the solver.
- The optic overlay is decorative and carries no real calibration.

## Acceptance criteria

- The sky shows real cloud structure and the scene is lit by it, not by a flat ambient term.
- Distance haze visibly separates the near lane, the backstop, and the distant ridges.
- Cast shadows are visible on the ground from range furniture and vegetation.
- Ground, lane, and rifle show normal-mapped surface detail rather than flat colour.
- The hip view contains connected first-person arms and a detailed scoped rifle, with the optic
  seated on the receiver and the arms closed on the stock.
- The active target is readable in the hip view and centred and legible under the optic, and a hit
  leaves a visible impact mark.
- No geometry is displaced from its declared position.
- Scene construction never causes the automated smoke test to capture a blank frame.
- All prior C and Python tests pass, plus new headless tests covering layout and terrain relief.
- A native GPU smoke test captures and inspects hip and scoped frames.
- `rg` confirms the GUI still contains no network client or server implementation.
