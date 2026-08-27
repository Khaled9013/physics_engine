# Ballistics Validation Reference

## Isolation statement

The expected values in `tests/ballistics/reference_cases.csv` were selected and recorded before reading `source/vcb_ballistics.h` or any production ballistics implementation. The runtime validator contains no independent trajectory solver and never uses the network.

## Contract assumptions used to build the oracle

All inputs and linear outputs use SI units. `drop_m` is positive below the line of sight. `wind_drift_m` is positive right. Elevation correction is positive up and equals `atan2(drop_m, range_m) * 1000`. Windage correction is positive right and opposes drift, so it equals `atan2(-wind_drift_m, range_m) * 1000`. Wind direction is travel-to degrees clockwise from shooter-to-target; 90 degrees is a full-value wind producing positive-right drift.

## Primary manufacturer table

Source: [RUAG Ammotec / SWISS P, .308 Win. Final SR, 8.4 g / 130 gr, April 2020](https://www.swiss-p.com/images/content/products/Factsheets/5120_308_Win._SWISS_P_Final_SR_8.4_g_-_130_gr_EN.pdf), page 2.

Published conditions:

- Muzzle velocity: 895 m/s.
- Published G1 BC: 0.2397 at 895 m/s; the sheet also publishes 0.2214 at 340 m/s and 0.1771 at 200 m/s.
- Atmosphere: 15 C, 1013.25 hPa, 0% humidity, sea level.
- Crosswind: 5 m/s.
- Zero: the 100 m row of the trajectory matrix.
- Sight height: 0.020 m, inferred from the 50 m and 100 m trajectory entries and the published time-of-flight values. The sheet does not state sight height explicitly.

The `swiss_p_308_final_sr` rows are direct transcriptions of velocity, time, wind drift, and the 100 m zero trajectory row. Sign and SI conversions are mechanical. Corrections are derived from the published linear values using the contract equations above.

Velocity, time, drift, and drift-derived windage are gating checks. RUAG vertical path and elevation remain printed diagnostics because the sheet omits sight height and uses measured velocity-dependent aerodynamics that the four-input constant-G1 contract cannot reproduce exactly.

This table is the authoritative external check, but it is not an exact constant-BC oracle. RUAG's table uses velocity-dependent measured coefficients while the product contract accepts one conventional G1 BC. Its published path and drift are rounded to centimetres. The acceptance tolerances are therefore fixed at the start of validation:

| Output | Tolerance |
|---|---:|
| Time of flight | max(0.040 s, 8%) |
| Impact velocity | max(35 m/s, 8%) |
| Drop | diagnostic only; sight height is unpublished |
| Wind drift | max(0.25 m, 15%) |
| Elevation correction | diagnostic only; derived from path |
| Windage correction | max(0.50 mrad, 15%) |

The diagnostic classification was made after the first run, not by widening a tolerance. At 600 m the solver produced 4.299913 m drop and 7.166399 mrad elevation versus RUAG's 3.14 m and 5.233333 mrad. The same run matched RUAG's time, velocity, drift, and windage. The frozen values remain in the CSV and every run prints the comparison.

## Supplemental constant-G1 table

Source: [ShootersCalculator ballistic trajectory calculator](https://shooterscalculator.com/ballistic-trajectory-chart.php), captured 2026-08-17 with this [fixed query](https://shooterscalculator.com/ballistic-trajectory-chart.php?pl=VCB%20reference&presets=&df=G1&bc=0.5&bw=168&vi=2624.67&zr=109.361&sh=1.9685&sa=0&ws=11.1847&wa=90&ssb=on&cr=1093.61&ss=100&chartColumns=Range~m%60Elevation~cm%60Elevation~MIL~FBFFF5%60Windage~cm%60Windage~MIL~FBFFF5%60Time~s%60Vel%5Bx%2By%5D~m%2Fs&lbl=&submitst=%20Create%20Graph%20).

Captured conditions:

- G1 BC: 0.500.
- Muzzle velocity: 2624.67 ft/s = 800.000 m/s.
- Sight height: 1.9685 in = 0.0500 m.
- Zero: 109.361 yd = 100.000 m.
- Atmosphere: International Standard Atmosphere at sea level, 59 F, 29.92 inHg, 50% humidity.
- Wind: 11.1847 mph = 5.000 m/s, full-value crosswind.
- Output step: 100 yd = 91.44 m; maximum captured row: 1000 yd = 914.40 m.

The calculator describes its values as approximations. [Its wind-angle help](https://shooterscalculator.com/notes/wind-angle.htm) uses a wind-from convention, so the fixture preserves the published magnitude and remaps the sign to this project's travel-to convention. The fixed acceptance tolerances account only for displayed table rounding and modest model/integration differences:

| Output | Tolerance |
|---|---:|
| Time of flight | 0.010 s absolute |
| Impact velocity | 2 m/s absolute |
| Drop | 0.020 m absolute |
| Wind drift | 0.010 m absolute |
| Elevation correction | 0.030 mrad absolute |
| Windage correction | 0.030 mrad absolute |

The observed worst constant-G1 absolute errors were 0.004921 s time, 0.570 m/s velocity, 0.007822 m drop, 0.002597 m drift, 0.010807 mrad elevation, and 0.010942 mrad windage. The fixed limits above retain at least 1.8x measured cross-model margin without allowing a material trajectory regression.

## Property and envelope oracle

The validator independently checks these non-tabular requirements across the full documented envelope:

- Every null pointer and every NaN or infinity is rejected as an invalid argument and clears a nonnull output.
- Every finite value just outside a bound is rejected as out of range; exact inclusive boundaries are distinguished from outside values.
- Zero wind and 0/180 degree axial wind produce zero drift and windage correction within floating-point noise.
- Mirrored 90/270 degree crosswinds preserve longitudinal outputs and negate drift/correction.
- Tailwind has lower time/drop and higher remaining velocity than matched headwind.
- Mirrored 45/315 and 135/225 degree winds preserve longitudinal outputs, negate lateral outputs, and remain below full-value crosswind drift.
- A deterministic 576-case matrix spans low/nominal/high profiles, 1/100/1000/2000 m, 0/100 m/s wind, and eight cardinal/oblique directions; it requires exactly 397 finite OK results, 179 cleared no-solution results, and zero other statuses.
- Time of flight is positive and strictly increases with range.
- Impact velocity is finite, positive, never exceeds muzzle velocity, and does not increase with range.
- Past the zero range, drop and both correction magnitudes are finite and nondecreasing for the nominal flat-fire profile.
- At every successful range, angular corrections match the linear outputs and sign contract.
- Repeated identical calls produce bitwise-identical status and outputs.

No test derives expected trajectory values from production implementation code.

## Reproduction

Normal build and run:

`make -C tests/ballistics clean run`

Sanitizer build flags: `-O1 -g -Wall -Wextra -Wpedantic -Werror -fno-omit-frame-pointer -fsanitize=address,undefined`; compile the two C sources in the Makefile and run with `tests/ballistics/reference_cases.csv`.

Final 2026-08-17 results:

- Normal: 2,268 checks, 0 failures, 0 compiler warnings.
- ASan plus UBSan: 2,268 checks, 0 failures, no sanitizer findings.
- Matrix: 576 cases; 397 OK, 179 no-solution with cleared output, 0 other statuses.
- Three complete normal runs produced SHA-256 `294a12a01efa336a8fc6c4aa6eb8f129284056bef6cbaa4cb258a3ce3908b53a`.

# 2026-08-18 G7 drag-family extension

## Isolation statement for this section

Every expected value, tolerance, and separation floor in this section was captured, converted, and written to `tests/ballistics/reference_cases_g7.csv` and to this document before reading the extended `source/vcb_ballistics.h`, before reading any part of `source/vcb_ballistics.c`, and before reading the `ballistics-core` agent log. The only pre-freeze information about the extension was the system engineer's announced contract: a `vcb_ballistics_drag_family_e` enum (`..._g1 = 0`, `..._g7 = 1`), the rename of `g1_ballistic_coefficient` to `ballistic_coefficient` with unchanged 0.05-2.0 bounds, a `drag_family` field appended after `zero_range_m` whose zero value is G1, and invalid-argument rejection with cleared output for an invalid family. The log entry ordering in `docs/agents/ballistics-validator.md` records this sequence.

## Primary G7 table (gating)

Source: [ShootersCalculator ballistic trajectory calculator](https://shooterscalculator.com/ballistic-trajectory-chart.php), captured 2026-08-18 with this [fixed G7 query](https://shooterscalculator.com/ballistic-trajectory-chart.php?pl=VCB%20G7%20reference&presets=&df=G7&bc=0.25&bw=168&vi=2624.67&zr=109.361&sh=1.9685&sa=0&ws=11.1847&wa=90&ssb=on&cr=1093.61&ss=100&chartColumns=Range~m%60Elevation~cm%60Elevation~MIL~FBFFF5%60Windage~cm%60Windage~MIL~FBFFF5%60Time~s%60Vel%5Bx%2By%5D~m%2Fs&lbl=&submitst=%20Create%20Graph%20).

The query is a deliberate one-parameter mirror of the accepted Phase 1 constant-G1 query: only `df` (G1 to G7) and `bc` (0.5 to 0.25) differ. Every other field, the chart column set, the atmosphere, and the output step are byte-identical, so the two fixtures are directly comparable.

Captured conditions, as reported by the page:

- Drag function: G7; BC 0.250; bullet weight 168 gr.
- Muzzle velocity: 2624.67 ft/s = 800.000 m/s.
- Sight height: 1.9685 in = 0.0500 m.
- Zero: 109.361 yd = 100.000 m.
- Shooting angle: 0 degrees.
- Atmosphere: sea level, 59 F, 29.92 inHg, 50% humidity, speed of sound 1116 ft/s.
- Wind: 11.1847 mph = 5.000 m/s at 90 degrees, full-value crosswind.
- Output step: 100 yd = 91.44 m; maximum captured row 1000 yd = 914.40 m.

Table exactly as printed by the page (the 0 m row is the muzzle row and is not used):

| Range (m) | Elevation (cm) | Elevation (MIL) | Windage (cm) | Windage (MIL) | Time (s) | Vel[x+y] (m/s) |
|---|---|---|---|---|---|---|
| 92 | 0.23 | -0.03 | 2.11 | 0.23 | 0.12 | 747 |
| 183 | -9.29 | 0.51 | 8.40 | 0.46 | 0.25 | 696 |
| 274 | -35.80 | 1.30 | 19.44 | 0.71 | 0.38 | 646 |
| 366 | -82.00 | 2.24 | 35.77 | 0.98 | 0.53 | 599 |
| 457 | -151.12 | 3.30 | 58.03 | 1.27 | 0.69 | 554 |
| 549 | -247.08 | 4.50 | 86.95 | 1.58 | 0.86 | 510 |
| 640 | -374.67 | 5.85 | 123.37 | 1.93 | 1.05 | 468 |
| 732 | -539.78 | 7.38 | 168.33 | 2.30 | 1.25 | 428 |
| 823 | -749.77 | 9.11 | 223.10 | 2.71 | 1.47 | 390 |
| 915 | -1014.41 | 11.09 | 289.33 | 3.16 | 1.72 | 354 |

Conversions to `tests/ballistics/reference_cases_g7.csv`, identical in form to the accepted Phase 1 constant-G1 fixture:

- `range_m` is the exact 100 yd multiple (91.44, 182.88, ... 914.40). The page rounds the range label to whole metres; the step itself is 100 yd.
- `time_of_flight_s` and `impact_velocity_mps` are transcribed as printed.
- `drop_m` = -(Elevation cm) / 100, because the page prints elevation positive above the line of sight and the contract prints drop positive below it.
- `wind_drift_m` = (Windage cm) / 100, magnitude preserved. The calculator's [wind-angle help](https://shooterscalculator.com/notes/wind-angle.htm) uses a wind-from convention, so as in Phase 1 the fixture keeps the published magnitude and remaps the sign into this project's travel-to convention, where 90 degrees is a full-value wind producing positive-right drift.
- `elevation_correction_mrad` is the printed Elevation MIL column.
- `windage_correction_mrad` = -(Windage MIL), because the contract's windage correction opposes drift.

Transcription and sign mapping were verified against the source's own independently printed angular columns: recomputing `atan2(drop_m, range_m) * 1000` and `atan2(-wind_drift_m, range_m) * 1000` from the converted linear values reproduces the printed MIL columns to within 0.0053 mrad at every row, which is inside the +/-0.005 mrad display rounding of a two-decimal MIL column. A sign or unit error in the conversion could not pass this check.

## Frozen G7 acceptance tolerances

Derived before any solver contact, from displayed rounding plus a modest cross-model margin, absolute-first:

| Output | Displayed rounding | Cross-model / range-label margin | Frozen tolerance |
|---|---|---|---:|
| Time of flight | +/-0.005 s | integration and atmosphere differences | 0.010 s |
| Impact velocity | +/-0.5 m/s printed, ~+/-0.15 m/s from the ft/s conversion | G7 drag-table interpolation near the transonic final rows | 3 m/s |
| Drop | +/-0.00005 m | ~0.013 m from the <=0.6 m range-label ambiguity at 914.40 m, plus model difference | 0.030 m |
| Wind drift | +/-0.00005 m | crosswind model difference | 0.015 m |
| Elevation correction | +/-0.005 mrad | follows the drop margin | 0.040 mrad |
| Windage correction | +/-0.005 mrad | follows the drift margin | 0.040 mrad |

The Phase 1 constant-G1 fixture uses 0.010 s, 2 m/s, 0.020 m, 0.010 m, 0.030 mrad, 0.030 mrad. The G7 limits above are equal on time and 1.3x to 1.5x wider on the remaining outputs. The widening is confined to what the G7 fixture adds over the G1 one: its final rows sit at roughly Mach 1.03 to 1.05, where independent implementations of the G7 drag table differ most, while the Phase 1 G1 BC 0.500 fixture stays above Mach 1.09 throughout. No tolerance here may be changed once the solver has been run; a failure is reported, not accommodated.

These limits remain tight enough to catch a family, unit, or sign error. The demonstration below uses the two captured tables:

- A complete family swap, that is G7 ignored and the same BC solved as G1, changes drop at 914.40 m by 9.62 m (321x the drop tolerance), velocity at 457.20 m by 181 m/s (60x), time at 914.40 m by 0.70 s (70x), and drift at 914.40 m by 3.49 m (233x). Every field at nearly every row detects it.
- The subtler error of implementing G7 as G1 with a rescaled BC is also caught. Against the already-frozen Phase 1 G1 BC 0.500 rows, the G7 BC 0.250 rows differ at 914.40 m by 0.0965 m drop (3.2x the drop tolerance), 19 m/s velocity (6.3x), 0.10 mrad elevation (2.5x), and 0.10 mrad windage (2.5x), and by 12 m/s velocity at 822.96 m and 4 m/s at 640.08 m.
- A centimetre-to-metre or metre-to-centimetre slip, a drop sign flip, or a drift sign flip is orders of magnitude outside every limit.

## Second anchor and cross-family consistency

No manufacturer publishes a static G7 trajectory table. Berger, Hornady, Lapua, Nosler and Sierra publish G7 *ballistic coefficients* and direct users to a calculator for the trajectory; searches for a published G7 drop/drift table returned only BC listings and calculator front ends. The Phase 1 RUAG-style manufacturer anchor therefore has no G7 counterpart, and this is recorded rather than substituted with a second table from the same engine.

The available independent anchor is a manufacturer-published paired BC for one bullet. [Berger Bullets, "Comparing Ballistic Coefficients (BCs)"](https://bergerbullets.com/nobsbc/comparing-ballistic-coefficients-bcs/) states that a bullet with a G7 BC of 0.279 above 3000 fps can have a G1 BC of 0.590 above 3000 fps, a G1-to-G7 ratio of 2.11. Two structural expectations follow, and both are frozen as checks:

- A G7 BC of 0.250 must behave close to, but not identically to, a G1 BC of roughly 0.50 in the supersonic near field. The captured G7 rows and the already-accepted Phase 1 G1 BC 0.500 rows agree to 0.0001 m of drop at 182.88 m and to 0.0005 m at 457.20 m, then diverge to 0.0965 m by 914.40 m as velocity decays. This is independent evidence that the captured G7 curve is the real G7 standard and not a relabelled or rescaled G1.
- Because the two families differ by roughly a factor of two in effective drag for the same BC number, a same-BC family swap must show metres of drop separation at long range, not centimetres.

## Family-separation source table (not gated row by row)

Source: the same calculator and the same fixed query with only `df=G1` substituted, captured 2026-08-18: [fixed G1 BC 0.25 query](https://shooterscalculator.com/ballistic-trajectory-chart.php?pl=VCB%20G1%20same%20bc&presets=&df=G1&bc=0.25&bw=168&vi=2624.67&zr=109.361&sh=1.9685&sa=0&ws=11.1847&wa=90&ssb=on&cr=1093.61&ss=100&chartColumns=Range~m%60Elevation~cm%60Elevation~MIL~FBFFF5%60Windage~cm%60Windage~MIL~FBFFF5%60Time~s%60Vel%5Bx%2By%5D~m%2Fs&lbl=&submitst=%20Create%20Graph%20). The page notes the sound barrier is crossed between its 457 m and 549 m rows.

| Range (m) | Elevation (cm) | Elevation (MIL) | Windage (cm) | Windage (MIL) | Time (s) | Vel[x+y] (m/s) |
|---|---|---|---|---|---|---|
| 92 | 0.31 | -0.03 | 4.27 | 0.47 | 0.12 | 695 |
| 183 | -11.51 | 0.63 | 17.98 | 0.98 | 0.26 | 599 |
| 274 | -46.40 | 1.69 | 43.49 | 1.58 | 0.43 | 512 |
| 366 | -112.95 | 3.09 | 83.39 | 2.28 | 0.62 | 435 |
| 457 | -223.18 | 4.88 | 140.11 | 3.06 | 0.85 | 373 |
| 549 | -392.57 | 7.15 | 214.11 | 3.90 | 1.11 | 329 |
| 640 | -637.65 | 9.96 | 302.89 | 4.73 | 1.41 | 300 |
| 732 | -974.22 | 13.32 | 404.00 | 5.52 | 1.72 | 280 |
| 823 | -1415.94 | 17.20 | 515.91 | 6.27 | 2.06 | 263 |
| 915 | -1976.48 | 21.61 | 638.04 | 6.98 | 2.42 | 249 |

These rows are deliberately not frozen as row-by-row gating expectations. Their terminal rows are subsonic, down to Mach 0.73, where the fixed absolute limits above are not defensible, and their role here is only to quantify how far apart the two families must be. Derived source separations, G1 minus G7 at equal BC 0.250:

| Range (m) | Drop separation (m) | Velocity separation (m/s) | Time separation (s) | Drift separation (m) | Elevation separation (mrad) |
|---:|---:|---:|---:|---:|---:|
| 274.32 | 0.1060 | 134 | 0.05 | 0.2405 | 0.39 |
| 457.20 | 0.7206 | 181 | 0.16 | 0.8208 | 1.58 |
| 640.08 | 2.6298 | 168 | 0.36 | 1.7952 | 4.11 |
| 914.40 | 9.6207 | 105 | 0.70 | 3.4871 | 10.52 |

## Frozen family-discrimination expectations

Solving one identical profile twice, changing only `drag_family`, with BC 0.250, muzzle velocity 800 m/s, sight height 0.0500 m, zero 100 m, and 5 m/s wind at 90 degrees:

1. Drop separation at 914.40 m is at least 5.0 m. Source value 9.6207 m; the floor sits 48% under the source and 167x above the 0.030 m drop tolerance.
2. Drop separation at 640.08 m is at least 1.0 m. Source value 2.6298 m.
3. Impact-velocity separation at 457.20 m is at least 80 m/s. Source value 181 m/s.
4. Time-of-flight separation at 914.40 m is at least 0.30 s. Source value 0.70 s.
5. Wind-drift separation at 914.40 m is at least 1.5 m. Source value 3.4871 m.
6. Direction, not just magnitude: at every range from 274.32 m outward, the G1 solution must drop more, fly longer, drift more, and retain less velocity than the G7 solution at the same BC. For the same BC number the G7 standard projectile is the lower-drag reference, so G7 must be the flatter trajectory. A selector wired backwards passes a magnitude-only test and fails this one.

Two further checks distinguish a true second drag family from a rescaled G1:

7. G7 at BC 0.250 and G1 at BC 0.500 must agree in drop to within 0.05 m at 182.88 m. Source difference 0.0001 m.
8. The same pair must differ in impact velocity by at least 10 m/s at 914.40 m. Source difference 19 m/s. Together with check 7 this rejects an implementation that merely doubles the BC and reuses the G1 curve.

## Frozen contract expectations for the extension

- `vcb_ballistics_drag_family_g1` is 0 and `vcb_ballistics_drag_family_g7` is 1.
- A profile whose `drag_family` was never written, that is a zero-initialised profile, must produce bitwise-identical status and outputs to the same profile with `drag_family` set explicitly to `vcb_ballistics_drag_family_g1`.
- Family values 2 and 255 must be rejected with the invalid-argument status and a cleared output, matching the existing invalid-input contract.
- `ballistic_coefficient` keeps the inclusive 0.05 to 2.0 bounds in both families; the bound checks are family-independent.
- Every previously frozen G1 row, property, wind assertion, and envelope count must still pass unchanged with `drag_family` set explicitly to G1.

## G7 reproduction and results

The harness now takes both fixtures:

`make -C tests/ballistics clean run`, which runs `vcb_ballistics_validator reference_cases.csv reference_cases_g7.csv`.

Sanitizer build flags are unchanged: `-O1 -g -Wall -Wextra -Wpedantic -Werror -fno-omit-frame-pointer -fsanitize=address,undefined`.

2026-08-18 results, gcc 13.3.0:

- Normal: 2,423 checks, 0 failures, 0 compiler warnings.
- ASan plus UBSan: 2,423 checks, 0 failures, no sanitizer findings.
- Matrix unchanged at 576 cases; 397 OK, 179 no-solution, 0 other.
- Three complete normal runs produced SHA-256 `a45437ae26846e7b0a82fe10a00f3116ffe209008be1da508510539bd81d2ee9`.

The 155 new checks are strictly additive; the previously accepted count of 2,268 is unchanged. They break down as 94 for the ten G7 fixture rows, 39 for family discrimination, 12 for invalid-family and family-independent BC bounds, and 10 for zero-initialised family equivalence.

Worst observed absolute error against the frozen G7 rows, and the margin each frozen tolerance retained:

| Output | Worst error | Range | Frozen tolerance | Margin |
|---|---:|---:|---:|---:|
| Time of flight | 0.004855 s | 182.88 m | 0.010 s | 2.06x |
| Impact velocity | 0.459 m/s | 274.32 m | 3 m/s | 6.54x |
| Drop | 0.012550 m | 914.40 m | 0.030 m | 2.39x |
| Wind drift | 0.003863 m | 914.40 m | 0.015 m | 3.88x |
| Elevation correction | 0.010456 mrad | 914.40 m | 0.040 mrad | 3.83x |
| Windage correction | 0.011589 mrad | 91.44 m | 0.040 mrad | 3.45x |

No tolerance was altered after the first run. The worst drop error, 0.01255 m at 914.40 m, sits almost exactly on the 0.013 m range-label ambiguity that was predicted when the limit was frozen.

Observed family separation against the frozen floors and the independent source values:

| Quantity | Range | Source separation | Observed | Frozen floor |
|---|---:|---:|---:|---:|
| Drop | 274.32 m | 0.1060 m | 0.106001 m | direction only |
| Impact velocity | 457.20 m | 181 m/s | 180.871 m/s | 80 m/s |
| Drop | 640.08 m | 2.6298 m | 2.627710 m | 1.0 m |
| Drop | 914.40 m | 9.6207 m | 9.605509 m | 5.0 m |
| Time of flight | 914.40 m | 0.70 s | 0.696884 s | 0.30 s |
| Wind drift | 914.40 m | 3.4871 m | 3.484426 m | 1.5 m |

The solver reproduces the independently captured family separation to within 0.2% at every checked range, and the direction checks hold at every range: at equal BC the G1 solution drops more, flies longer, drifts more, needs more elevation, and retains less velocity than the G7 solution.

The rescaled-G1 discrimination also holds. G7 at BC 0.250 and G1 at BC 0.500 differ by 0.000044 m of drop at 182.88 m, inside the 0.05 m near-field agreement expectation, and by 18.944 m/s of impact velocity at 914.40 m, above the 10 m/s downrange separation expectation and close to the 19 m/s source value. A G7 family implemented by reusing the G1 curve with a doubled BC would fail the second check.

# 2026-08-18 Shipped G7 model limitation

Provenance: the fit, knot, step-sensitivity, and trajectory-cost figures in this section are from the accepted Phase 6 numerical audit recorded in the Task 2 entries of `docs/agents/ballistics-review.md`, added here on the reviewer's Step 16 return. The reachability figures at the end of the section are from the reviewer's separate Step 17 envelope sweep. Neither set was measured by this validator. They are restated here because the validation reference is the durable, role-independent artifact and because the acceptability conclusion below is conditional on parameters that can change.

The shipped G7 retardation law is the pinned upstream's coarse **9-segment** piecewise fit, against **41** segments for G1. It contains knot discontinuities in retardation at the segment boundaries. The two largest are **+55.385% at 1110 fps (338.33 m/s)** and **-11.594% at 1260 fps (384.05 m/s)**, and both sit inside the long-range impact band, so they are reachable in normal use rather than only at envelope extremes.

Trajectory-level cost, measured across ten knot-crossing geometries at the 0.5 m production step:

- Worst elevation deviation **0.0129089 mrad**. Against the product's boresight granularity of 0.128 mrad per pixel, derived from a 12 um IFOV over 75 mm optics, that is **0.101 px** — below one pixel of aiming adjustment.
- An 801-point muzzle-velocity sweep across the knot returned all statuses defined, all outputs finite, and **zero monotonicity breaks**, so the discontinuity produces no jump, no mis-ordered segment scan, and no no-solution artifact.

G7 is measurably more **step-sensitive** than G1, by **3x to 8x at matched geometry** under controlled comparison. This is recorded because it interacts with any future change to the integration step: a step change that is harmless on the G1 path is not automatically harmless on the G7 path.

The conclusion that the coarse fit is acceptable is **conditional on two things**, and both must be restated whenever it is relied on:

1. It holds **at the 0.5 m fixed integration step**.
2. It holds **within the declared input envelope**.

Changing either invalidates the conclusion and requires re-measurement. The acceptability figure above is not a property of the G7 table alone; it is a property of that table at that step inside that envelope.

Reachability, measured by the reviewer's Step 17 envelope sweep over muzzle velocities 350 to 1500 m/s in 5 m/s steps, which is 231 muzzles, against ranges 100 to 2000 m:

| G7 BC | Muzzles with an in-envelope knot crossing |
|---:|---:|
| 2.00 | 17 of 231 |
| 1.00 | 40 of 231 |
| 0.50 | 95 of 231 |
| 0.25 | 231 of 231 |
| 0.05 | 231 of 231 |

The knot is therefore **reachable at every BC in the declared envelope**, and reachability rises steeply as BC falls. Even at the 2.0 BC ceiling, 17 muzzle velocities cross it; at BC 1.00 the earliest crossing occurs at just 150 m from a 350 m/s muzzle. **No BC grants an exemption from the two validity conditions above.** The frozen G7 fixture in this document uses BC 0.250 and therefore crosses the affected band at its long ranges.

An earlier revision of this section stated that the knot was unreachable at G7 BC 1.0 and mattered only at BC 0.50 and below. That claim is **withdrawn**: it was extrapolated beyond the audit's measured geometries, which spanned BC 0.05 to 0.50 and never included BC 1.0, and the sweep above contradicts it directly.

