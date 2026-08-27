# Ballistics Validator Agent Log

## 2026-08-17 - Step 1: Project context and isolation setup

- Changed: created `docs/agents/ballistics-validator.md` as this role's recovery log.
- Why: record every completed validator step and preserve the required independent-oracle boundary.
- Verified: matched the workspace to the VCB project in `/home/khd/ai_mind`; read the VCB project map and required project notes; confirmed no repository `AGENTS.md` exists; recorded the pre-existing dirty state (`lvgl_support/vcb_menu.h`, `vcb-gui-sim`, and `docs/agents/`).
- Still open: survey host build/test conventions without reading any ballistics implementation; define and document independent published reference expectations before opening the public solver header.

## 2026-08-17 - Step 2: Host build and test convention survey

- Changed: no production or test files; surveyed root layout, build metadata, and existing test locations while excluding `*ballistic*` paths.
- Why: choose a host-runnable validator layout without contaminating the independent oracle with production implementation details.
- Verified: the embedded root has no top-level CMake test harness; the simulator is a dirty submodule and was not modified; existing host tests are isolated under component-owned directories. Chosen convention is a self-contained `tests/ballistics/` C harness plus Makefile, with generated artifacts outside the source tree.
- Still open: pin independent published/reference expected numbers and tolerances; production `vcb_ballistics` source and header remain unread.

## 2026-08-17 - Step 3: Independent oracle and tolerances frozen

- Changed: added `docs/ballistics-validation-reference.md` and `tests/ballistics/reference_cases.csv` with 22 expected rows covering time, velocity, path/drop, wind drift, and both mrad corrections.
- Why: freeze external expected values before exposing the validator to the production interface.
- Verified: transcribed 12 rows from the RUAG/SWISS P .308 Final SR factsheet and 10 rows from a fixed constant-G1 calculator query; documented exact source URLs, conditions, conversions, sign mapping, and immutable per-output tolerances. Added independent invalid-input, boundary, symmetry, monotonicity, consistency, and repeatability expectations.
- Still open: lift the isolation barrier by reading only the public header, adapt a C harness to that API, and compare actual solver output. The implementation source remains unread.

## 2026-08-17 - Step 4: Isolation barrier lifted for the public contract

- Changed: no repository code; accepted the finalized public API contract supplied by `ballistics-core` after the oracle was frozen.
- Why: bind tests to the production type and status names without seeing how the solver computes results.
- Verified: the contract defines SI profile/environment/solution structs, `vcb_ballistics_solve`, status values, sign conventions, inclusive input bounds, failure-output clearing, and `[0,360)` wind direction. Attempted to read `source/vcb_ballistics.h`, but it had not landed yet.
- Still open: compile the harness once the header exists. No `vcb_ballistics` implementation file has been opened or read.

## 2026-08-17 - Step 5: Standalone validator authored

- Changed: added `tests/ballistics/test_vcb_ballistics.c` and `tests/ballistics/Makefile`.
- Why: provide a network-free host executable that checks every solution field against the frozen CSV and exercises contract properties across invalid, boundary, wind-symmetry, and range-sweep cases.
- Verified: the harness contains no trajectory oracle, prints expected/actual values for all 22 reference rows, enforces failure-output clearing, and builds with C11 plus `-Wall -Wextra -Wpedantic -Werror`. Build artifacts are directed to `/tmp/vcb-ballistics-validator`.
- Still open: compile and run when `source/vcb_ballistics.h/.c` land; route any mismatch through reviewer. Production implementation remains unread.

## 2026-08-17 - Step 6: Final public header binding and clean build

- Changed: adapted `tests/ballistics/test_vcb_ballistics.c` to the public enum typedef `vcb_ballistics_status_e`; no oracle values or tolerances changed.
- Why: the finalized public header uses `_e` where the pre-header contract message used `_t`.
- Verified: read only `source/vcb_ballistics.h`; built the validator against `source/vcb_ballistics.c` with C11, `-Wall -Wextra -Wpedantic -Werror`, and no warnings. The implementation source was not opened or read.
- Still open: run the frozen reference and property suite.

## 2026-08-17 - Step 7: First-run reference triage

- Changed: added range-sweep actual/derived output to `tests/ballistics/test_vcb_ballistics.c`; preserved all frozen CSV values and tolerances.
- Why: quantify 20 first-run failures rather than guessing at their cause.
- Verified: 10 external constant-G1 rows matched all six fields; RUAG matched time, velocity, drift, and windage but its under-specified vertical path diverged at long range. Seven angular property failures were caused by the validator using a small-angle ratio instead of geometric `atan2`, reaching 0.051639 mrad error at 2000 m.
- Still open: apply the reviewer's approved RUAG diagnostic classification and correct the angular oracle without tolerance widening.

## 2026-08-17 - Step 8: Reference and property suite passing

- Changed: `docs/ballistics-validation-reference.md` now specifies `atan2` angular conversion and explicitly classifies RUAG vertical path/elevation as non-gating diagnostics; `tests/ballistics/test_vcb_ballistics.c` applies the same rules.
- Why: RUAG does not publish the sight height needed to reproduce its path, while the fixed external constant-G1 table supplies gating values for all six outputs.
- Verified: clean rebuild and offline run completed 442 checks with 0 failures. The 1–2000 m sweep, wind symmetry, null/NaN/Inf, out-of-range values, exact boundaries, failure clearing, no-solution, angular consistency, monotonicity, and repeatability passed.
- Still open: sanitizer run and reviewer gate.

## 2026-08-17 - Step 9: Sanitizer verification

- Changed: no source files; compiled a separate `/tmp/vcb-ballistics-validator/vcb_ballistics_validator_san` binary.
- Why: check the production solver and independent harness for memory errors and undefined behavior.
- Verified: AddressSanitizer plus UBSan run completed all 442 checks with 0 failures and no sanitizer findings.
- Still open: quantify maximum angular delta, finish reproducibility checks, and submit to reviewer.

## 2026-08-17 - Step 10: Numerical delta and reproducibility audit

- Changed: no source files; built a temporary stdin-only angular checker and captured three complete validator outputs under `/tmp/vcb-ballistics-validator`.
- Why: provide exact error bounds and prove deterministic host results before review.
- Verified: maximum elevation and windage conversion absolute and relative deltas were all exactly 0 across the 1–2000 m sweep. Three complete runs each reported 442 checks and 0 failures and produced the identical SHA-256 `5599df169c5f332ebefb09f4a73713e8dc0ad352a4fdc7666d2e133048dc9427`.
- Still open: reviewer verdict only; no validator implementation item remains open.

## 2026-08-17 - Step 11: Source convention link completed

- Changed: added the exact ShootersCalculator wind-angle help URL to `docs/ballistics-validation-reference.md`.
- Why: make the fixture's wind-from to travel-to sign remapping independently traceable.
- Verified: the reference document now links the published fixed query, RUAG factsheet, and wind convention separately; runtime tests remain network-free.
- Still open: reviewer verdict only.

## 2026-08-17 - Step 12: Reviewer gate failure received

- Changed: no implementation files; recorded the reviewer verdict and reopened validator work.
- Why: the gate found permissive constant-G1 tolerances, incomplete wind-vector assertions, and insufficient low/nominal/high interaction coverage.
- Verified: reviewer accepted isolation, source transcription, RUAG diagnostic handling, strict build, sanitizer run, and reproducibility; it returned exact issue/why/fix guidance for the three remaining defects.
- Still open: tighten all-field constant-G1 limits, add axial/oblique physics checks, add a deterministic full-envelope matrix with status accounting, rerun, and resubmit.

## 2026-08-17 - Step 13: Constant-G1 gates tightened

- Changed: tightened constant-G1 limits in `tests/ballistics/test_vcb_ballistics.c` and `docs/ballistics-validation-reference.md` to fixed absolute bounds: 0.010 s, 2 m/s, 0.020 m drop, 0.010 m drift, and 0.030 mrad for each angle.
- Why: prevent a 4–5% trajectory regression from passing under relative tolerances.
- Verified: documented worst observed absolute errors of 0.004921 s, 0.570 m/s, 0.007822 m drop, 0.002597 m drift, 0.010807 mrad elevation, and 0.010942 mrad windage; every new limit retains at least 1.8x measured margin.
- Still open: wind-vector and full-envelope matrix assertions, then full rerun.

## 2026-08-17 - Step 14: Wind-vector physics assertions added

- Changed: expanded `test_zero_and_mirrored_wind` in `tests/ballistics/test_vcb_ballistics.c` with 0/180 head-tail ordering and 45/315 plus 135/225 oblique checks.
- Why: detect axial convention swaps and incorrect sine/cosine wind decomposition that cardinal lateral signs alone cannot catch.
- Verified: a public-API probe at 1000 m showed tail/head time 1.925548/1.953903 s, velocity 355.264/344.655 m/s, drop 12.820535/13.169427 m, mirrored oblique signs, and oblique drift below the 3.447711 m full-value drift.
- Still open: compile the new assertions, add the 576-case full-envelope matrix, and rerun.

## 2026-08-17 - Step 15: Full-envelope interaction matrix added

- Changed: added `test_full_envelope_matrix` to `tests/ballistics/test_vcb_ballistics.c` and registered it in `main`.
- Why: cover low/nominal/high velocity, BC, sight height, zero, 1–2000 m range, 0/100 m/s wind, and cardinal/oblique directions in interaction rather than isolated boundary calls.
- Verified: the deterministic 576-case public-API probe established per-profile expectations totaling 397 OK, 179 no-solution, and 0 other statuses. The harness requires finite/positive and angular-consistent OK outputs, cleared no-solution outputs, exact profile/total status counts, and rejects every other status.
- Still open: clean build, normal run, sanitizer run, reproducibility hashes, documentation update, and reviewer resubmission.

## 2026-08-17 - Step 16: Expanded suite fully verified

- Changed: added explicit zero/negative requested-range checks and final reproduction evidence to `docs/ballistics-validation-reference.md`.
- Why: close the remaining contract edge and make the expanded reviewer-fix run independently reproducible.
- Verified: clean C11 `-Werror` build and normal run completed 2,268 checks with 0 failures; ASan plus UBSan completed 2,268 checks with no findings; the matrix reported 576/397/179/0 total/OK/no-solution/other; three outputs shared SHA-256 `294a12a01efa336a8fc6c4aa6eb8f129284056bef6cbaa4cb258a3ce3908b53a`.
- Still open: reviewer verdict only; every validator implementation and documentation item is closed.

## 2026-08-17 - Step 17: Reviewer gate passed

- Changed: no implementation files; recorded the final independent reviewer verdict.
- Why: complete the required review loop after resolving every returned issue.
- Verified: reviewer independently reran clean C11 `-Werror` and ASan plus UBSan suites at 2,268 checks with 0 failures, reproduced matrix counts 576/397/179/0, and accepted isolation, provenance, strict gates, wind assertions, and RUAG diagnostic classification.
- Still open: none. Preserve the dated external calculator capture together with its frozen CSV and documented conditions.

## 2026-08-18 - Step 18: Phase 6 G7 oracle frozen before implementation contact

- Changed: added the dated `2026-08-18 G7 drag-family extension` section to `docs/ballistics-validation-reference.md` and created `tests/ballistics/reference_cases_g7.csv` with 10 gating G7 rows.
- Why: repeat the Phase 1 sequence exactly and freeze every G7 expected value, tolerance, and separation floor before reading the extended header, the implementation, or the core agent log.
- Verified: captured a fixed, dated, URL-recorded ShootersCalculator query that is a one-parameter mirror of the accepted Phase 1 query (`df` G1 to G7, `bc` 0.5 to 0.25, everything else byte-identical), plus the matching `df=G1` BC 0.25 capture used only to quantify family separation. Conversions were checked against the source's own printed MIL columns and reproduce them to within 0.0053 mrad, inside the +/-0.005 mrad display rounding, which independently confirms the sign and unit mapping. Froze absolute tolerances of 0.010 s, 3 m/s, 0.030 m drop, 0.015 m drift, and 0.040 mrad per angle, and eight family-discrimination expectations including a direction check and two G7-versus-rescaled-G1 checks. Recorded that no manufacturer publishes a static G7 trajectory table and used Berger's published paired BC (G7 0.279 to G1 0.590) as the independent structural anchor instead.
- Still open: read only the extended `source/vcb_ballistics.h`, adapt the harness to the rename, add the G7 cases, and run. `source/vcb_ballistics.c` and the `ballistics-core` log remain unread.

## 2026-08-18 - Step 19: Isolation barrier lifted and harness extended

- Changed: adapted `tests/ballistics/test_vcb_ballistics.c` to the renamed `ballistic_coefficient` field with an explicit `drag_family` on every existing fixture, matrix, and boundary case, added the G7 fixture profile and tolerance set, added `test_drag_family_discrimination`, `test_invalid_drag_family`, and `test_zero_initialised_family_is_g1`, and updated `tests/ballistics/Makefile` to pass both CSVs.
- Why: bind the accepted suite to the extended contract without altering any frozen expectation.
- Verified: read only `source/vcb_ballistics.h`, which had landed and matched the announced contract exactly, including `drag_family` appended after `zero_range_m` and G1 at zero. `source/vcb_ballistics.c` and the `ballistics-core` log were not opened. The only edits to existing test code were the field rename and the explicit G1 family initialiser.
- Still open: clean strict build, full run, sanitizer run, and determinism evidence.

## 2026-08-18 - Step 20: G7 extension verified green against the frozen oracle

- Changed: added the G7 reproduction and results subsection to `docs/ballistics-validation-reference.md`.
- Why: record the measured margins, separations, and sanitizer evidence for review.
- Verified: clean C11 `-Werror` build with no warnings and 2,423 checks with 0 failures; ASan plus UBSan produced 2,423 checks with 0 failures and no findings; three complete runs produced the identical SHA-256 `a45437ae26846e7b0a82fe10a00f3116ffe209008be1da508510539bd81d2ee9`. The 155 new checks are strictly additive over the accepted 2,268, verified by an independent per-test count that reconciles exactly. Every frozen G7 tolerance held with at least 2.06x margin, worst case 0.012550 m of drop at 914.40 m against the 0.030 m limit. Family separation matched the independently captured source values to within 0.2% at all four ranges, all direction checks held, and the rescaled-G1 discrimination held at 0.000044 m near-field drop agreement and 18.944 m/s downrange velocity separation. No tolerance or expected value was changed after any run.
- Still open: reviewer verdict only. No mismatch was found, so nothing is routed to core.

## 2026-08-18 - Step 21: Reviewer documentation FAIL closed

- Changed: added the dated `2026-08-18 Shipped G7 model limitation` section to `docs/ballistics-validation-reference.md`. No code, CSV, or tolerance changed.
- Why: the reviewer returned the combined Phase 6 submission as a one-item documentation FAIL. The coarse-fit limitation existed only in the audit's session log, while the validation reference is the durable role-independent artifact, and the acceptability conclusion is conditional on parameters that can change.
- Verified: the section records the 9-segment G7 fit against 41 for G1, the two largest knot discontinuities at +55.385% at 1110 fps or 338.33 m/s and -11.594% at 1260 fps or 384.05 m/s, the 0.0129089 mrad worst elevation deviation restated as 0.101 px against the 0.128 mrad per pixel boresight granularity from a 12 um IFOV over 75 mm optics, the clean 801-point muzzle-velocity sweep, the 3x to 8x G7 step sensitivity over G1, the two explicit conditions of the 0.5 m fixed step and the declared envelope with re-measurement required if either changes, and the reachability note that the knot is unreachable at G7 BC 1.0 and matters at BC 0.50 and below. Cross-checked every figure against the Task 2 entries of `docs/agents/ballistics-review.md` before writing, and attributed them to the audit role rather than presenting them as my own measurements. Oracle isolation was already discharged, so reading the audit log was permitted at this point.
- Still open: reviewer re-read of the single new section.

## 2026-08-18 - Step 22: Reachability claim corrected

- Changed: replaced the reachability sentence in the `2026-08-18 Shipped G7 model limitation` section of `docs/ballistics-validation-reference.md` with the measured envelope sweep, and corrected that section's provenance line to separate its two sources. No code, CSV, or tolerance changed.
- Why: the reviewer's re-review found the claim that the knot is unreachable at G7 BC 1.0 and matters only at BC 0.50 and below to be factually wrong. I had restated it faithfully from the audit log, but the audit never measured BC 1.0; its geometries spanned BC 0.05 to 0.50, so the claim was an extrapolation beyond measured ground.
- Verified: the section now carries the reviewer's Step 17 sweep over muzzle velocities 350 to 1500 m/s in 5 m/s steps, which is 231 muzzles, against ranges 100 to 2000 m, with in-envelope knot crossings at 17 of 231 muzzles for BC 2.00, 40 of 231 for BC 1.00 with the earliest crossing at 150 m from a 350 m/s muzzle, 95 of 231 for BC 0.50, and 231 of 231 for both BC 0.25 and BC 0.05. It states that the knot is reachable at every BC in the declared envelope, that reachability rises as BC falls, and that no BC grants an exemption from the two validity conditions. The withdrawn claim is recorded explicitly rather than silently deleted, so the correction is auditable. The connecting line about the frozen BC 0.250 fixture crossing the affected band was retained. The provenance line now attributes the fit, knot, step-sensitivity, and trajectory-cost figures to the audit's Task 2 entries and the reachability figures to the reviewer's Step 17 sweep, and states that neither set was measured by this validator.
- Still open: reviewer re-read of the corrected sentence.
