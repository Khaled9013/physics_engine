# Code Review — Ballistics Physics Engine

**Date:** 2026-07-23
**Overall score:** 88/100
**Verdict:** No critical or high-severity bugs found. All findings are low-severity edge cases or design smells; nothing affects the correctness of a normally configured trajectory.

## Scope

Reviewed in depth: the C engine (core, physics models, equations, integrators, stop
conditions, registries, output, port layer), the `ballistic_cli` application, and the
GUI's simulation layer (`cli_bridge.py`, `worker.py`, `models.py`). The GUI rendering
and Qt UI code (`apps/ballistics_gui/render`, `apps/ballistics_gui/ui`) was only
skimmed.

Verification performed during review: full build plus all 3 CTest targets pass
(`ballistics_unit_tests`, `ballistics_integration_tests`, `ballistics_cli_option_tests`).
The Python GUI suite was not rerun (pytest unavailable in the review environment).

## Verified correct

- **Drag** (`src/equations/aerodynamic_drag_equation.c:96`):
  `F = -1/2 * rho * Cd * A * |v_rel| * v_rel` with `v_rel = v - wind` — standard
  quadratic drag correctly opposing air-relative motion.
- **Gravity / Newton's second law**: `F = m*g`, default `g = -9.80665 z`; `a = F/m`
  with mass validated positive.
- **Integrators**: classic RK4 coefficients exact (`src/integrators/rk4_integrator.c:135-140`);
  Euler correct. Convergence tests measure empirical order (RK4 >= 3.5, Euler ~ 1).
- **Ground impact** (`src/stop_conditions/ground_stop_condition.c:57-68`): linear
  interpolation within the bracketing step, alpha clamped to [0,1], z pinned to ground.
  The stop-condition wrapper (`src/interfaces/stop_condition_interface.c:21-33`)
  pre-initializes decisions and post-validates final states, so early-return paths are safe.
- **Numerical hygiene**: `hypot` for magnitudes, overflow-safe normalization
  (`src/math/vector3.c:99-106`), non-finite checks at every boundary, CMake rejects
  unsafe FP flags (`cmake/CompilerWarnings.cmake:24`).
- **Python bridge** (`apps/ballistics_gui/simulation/cli_bridge.py`): shell-free
  argument list, pinned C locale, subprocess timeout, CSV header/column/finiteness
  validation, sample-count cross-check against the summary line.

## Findings (ranked)

### 1. Dead config fields — validated but never enforced (design, medium)

`BallisticsSimulationConfig.maximum_distance_metres` and `.ground_height_metres`
(`include/ballistics/simulation_config.h:13-14`) are checked by
`ballistics_simulation_config_validate` but never read by the engine. The actual
limits come from independently constructed stop conditions, so there are two sources
of truth. The CLI happens to pass the same values to both, but a library consumer can
set config fields that silently do nothing, or wire up stop conditions that disagree
with the config.

**Fix:** have the simulation construct/check stop conditions from the config, or
delete the fields.

### 2. Max-time stop condition can turn a graceful stop into a hard error (robustness, low)

Unlike its three siblings, `maximum_time_evaluate`
(`src/stop_conditions/maximum_time_stop_condition.c:33-53`) never validates the
states before interpolating. If the state goes non-finite on the same step that
crosses max time, it interpolates garbage, the wrapper's post-check returns
`NUMERICAL_ERROR`, and `evaluate_stop_conditions` (`src/core/simulation.c:92-95`)
aborts the whole run — discarding the graceful `INVALID_STATE` stop the
invalid-state condition had already produced in the same loop.

**Fix:** add the same `ballistics_projectile_state_validate` guard its siblings use.

### 3. Duplicate timestamped samples on same-time stops (data quality, low)

When a stop decision lands at the *previous* time — an initial state below ground
(`src/stop_conditions/ground_stop_condition.c:40-48`) or an invalid-state stop
(`src/stop_conditions/invalid_state_stop_condition.c:18-21`) —
`src/core/simulation.c:247` appends a second sample at a timestamp already present in
the result. Consumers computing `dt = t[i+1] - t[i]` get a zero interval; the CSV
gets a duplicated row.

**Fix:** skip the append (or replace the prior sample) when
`decision.final_time_s` equals the last appended sample time.

### 4. Integrators don't check the final combined state (robustness, very low)

Both integrators verify stage derivatives are finite but not the final `out_state`
(`src/integrators/euler_integrator.c:51`, `src/integrators/rk4_integrator.c:135-140`);
an overflow in the last combination escapes as `+inf` with `STATUS_OK`. The CLI
always registers the invalid-state stop condition so it is caught there, but a direct
library user who omits that condition gets silent inf propagation until some later
validation trips with a confusing `INVALID_ARGUMENT`.

**Fix:** validate `out_state` finiteness before returning from the step functions.

### 5. Default 2 m/s crosswind (footgun, low)

Both the CLI defaults (`apps/ballistic_cli/cli_options.c:183`) and the GUI defaults
(`apps/ballistics_gui/simulation/models.py:41`) ship `wind_y = 2.0`. A
default-settings run drifts laterally, which a new user will likely read as a bug.

**Fix:** default to zero wind, or state the synthetic-scenario intent in the usage text.

### 6. Port mutex is a no-op that reports success (API, very low)

`ballistics_port_mutex_lock/unlock` on Linux (`src/port/linux_port.c:100-108`) return
success without providing any mutual exclusion. Nothing calls them today, so it is
dead-but-misleading API surface.

**Fix:** back it with pthreads, or document the library as single-threaded and drop it.

### 7. CSV precision is not round-trip (informational)

`%.9e` (`include/ballistics/output/csv_trajectory_writer.h:9`) gives ~10 significant
digits — not round-trip for IEEE-754 doubles. The CSV is the sole data channel to the
GUI. Fine for plotting; bump to `%.17g` if bit-exact fidelity ever matters.

### 8. Max-distance stop doesn't interpolate (consistency, very low)

`maximum_distance_evaluate` (`src/stop_conditions/maximum_distance_stop_condition.c:38-44`)
stops at the overshooting state rather than interpolating back to the boundary the way
the ground and time conditions do, so reported range can exceed the configured limit
by up to one step's travel.

## Score breakdown

Scored against production-quality simulation-library standards.

| Dimension | Score | Reasoning |
|---|---|---|
| Physics correctness | 19/20 | All force/integration math exact; -1 for non-interpolating max-distance stop |
| Numerical rigor | 18/20 | Strong hygiene throughout; -2 for unchecked final integrator state and CSV precision |
| API / architecture | 16/20 | Clean layering, ports, registries, vtables; -4 for dead config fields and no-op mutex |
| Robustness / error handling | 17/20 | Consistent status discipline; -3 for findings 2 and 3 |
| Testing | 18/20 | Measured convergence order, analytic references, determinism, sanitizers; -2 for untested edge paths behind findings 2-3 |
| **Total** | **88/100** | Fix findings 1-3 for ~92-93 |
