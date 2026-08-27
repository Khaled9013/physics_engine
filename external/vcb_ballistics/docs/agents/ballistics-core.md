# Ballistics Core Agent Log

## 2026-08-17 - Step 1: Survey and interface design

- Changed: `docs/agents/ballistics-core.md`; defined the recovery log and surveyed `source/`, `lvgl_support/`, `.cproject`, `Debug/source/subdir.mk`, profile storage, simulator discovery, compiler availability, and current Git state.
- Why: isolate the pure solver from UI, I/O, persistence, generated assets, and unrelated dirty files before choosing its contract.
- Verified: the target discovers `source/*.c` through the Eclipse source entry; the checked-in Debug make fragment is generated and currently stale; host GCC is available; `arm-none-eabi-gcc` is unavailable; `lvgl_support/ballistic.c` is an image asset, not a solver; existing dirty changes are `lvgl_support/vcb_menu.h`, `vcb-gui-sim`, and other agents' logs.
- Decision: add `source/vcb_ballistics.[ch]` with SI inputs, conventional G1 BC, no heap/I/O/global mutable state, fixed bounded integration, travel-to wind direction clockwise from line of sight, and explicit signed drop/drift/correction outputs.
- Envelope: muzzle velocity 50-1500 m/s, G1 BC 0.05-2.0, sight height 0-0.20 m, zero range 10-1000 m, requested range 1-2000 m, and wind 0-100 m/s; a low-angle zero must exist below 5 degrees.
- Context: the VCB vault was last reviewed before the current 2026-08-17 repository revision and has no ballistics task/subproject, so repository code and this assignment control current implementation details.
- Still open: implement the header, G1 model, low-angle zero solver, trajectory integrator, and focused tests; confirm simulator/target integration is owned by later phases.

## 2026-08-17 - Step 2: Public contract and numerical implementation

- Changed: `source/vcb_ballistics.h`; added explicit profile, environment, result, bounds, signed-output conventions, status values, and `vcb_ballistics_solve()`.
- Changed: `source/vcb_ballistics.c`; added the standard G1 piecewise retardation model, calm-condition low-angle zero bisection, wind-relative 3-axis point-mass dynamics, and fixed-distance RK4 integration.
- Why: provide a deterministic target/simulator solver with no heap, I/O, mutable globals, UI dependencies, or profile-storage coupling.
- Verified: the G1 coefficient set was cross-checked against the maintained GNU Ballistics drag-function source; every public scalar and intermediate state has finite/range guards; output is zeroed before validation; integration is capped from the requested range and fixed step; zero search is capped at 24 iterations and 5 degrees.
- Decision: G1 BC represents the conventional reference-projectile coefficient at standard atmosphere; wind is an air-velocity vector, direction is travel-to degrees clockwise from the target line, and the stored zero is solved in calm conditions so live wind cannot redefine boresight.
- Still open: compile with strict warnings/sanitizers, add focused behavior and rejection tests, measure 0.5 m step convergence against finer steps, and capture actual numeric results.

## 2026-08-17 - Step 3: Strict compile and focused behavior verification

- Changed: no repository code; compiled `source/vcb_ballistics.c` and an ephemeral isolated harness with host GCC plus AddressSanitizer and UndefinedBehaviorSanitizer.
- Why: verify warnings, memory/undefined behavior, deterministic output, sign contracts, zeroing on failure, and basic physical invariants before convergence work.
- Verified: strict core compile passed with `-std=c99 -Wall -Wextra -Werror -Wconversion -Wdouble-promotion -Wshadow -pedantic`; sanitizer harness passed deterministic repeat, 100 m zero, no-wind drift, opposing crosswinds, headwind/tailwind ordering, null, zero-range, NaN, and stale-output rejection checks.
- Results: calm 100 m gave TOF 0.131063730 s, impact 727.752075 m/s, drop -0.000000141 m; 10 m/s right crosswind at 500 m gave TOF 0.810675085 s, impact 478.272186 m/s, drop 2.126136 m, drift 1.856746 m, elevation 4.252245 mrad, windage -3.713474 mrad.
- Still open: measure step-size convergence across nominal and severe/transonic profiles, run broader envelope/error sweeps, and complete reviewer handoff.

## 2026-08-17 - Step 4: Step convergence and envelope sweep

- Changed: no repository code; compiled 0.5 m, 0.25 m, and 0.125 m fixed-step variants plus an ephemeral sanitizer envelope harness.
- Why: quantify production-step numerical error and exercise finite boundary values across the declared input envelope.
- Verified: all three convergence cases solved at all step sizes; a 375-case ASan/UBSan sweep across five profiles, five ranges, three wind speeds, and five directions completed with 328 valid solutions and 47 expected low-angle/unreachable `no_solution` results, with no numeric errors or sanitizer findings.
- Results: worst 0.5 m versus 0.125 m difference was the severe 900 m/s, BC 0.1, 800 m, 40 m/s wind case: 0.000010729 s TOF, 0.000275 m/s impact velocity, 0.000236512 m drop, 0.000190735 m drift, 0.000297546 mrad elevation, and 0.000244141 mrad windage.
- Still open: inspect the final diff, add standard-atmosphere wording to the public contract, rerun strict verification, and submit through reviewer.

## 2026-08-17 - Step 5: Embedded resource check

- Changed: no repository code; compiled an optimized benchmark and size/stack-analysis objects.
- Why: confirm the bounded no-heap design has practical CPU, flash, and stack costs before integration.
- Verified: 100 nominal 500 m solves completed in 0.100152 s on the host, or 1001.520 us per solve; `-Os` object size was 3827 bytes with zero data/BSS, and only `vcb_ballistics_solve` was globally exported.
- Results: host static stack reports peaked at 240 bytes for `vcb_ballistics_integrate`, 128 bytes for `vcb_ballistics_solve`, and 64 bytes for the derivative helper; target timing and final linked stack remain integration-phase measurements because the ARM toolchain is unavailable here.
- Still open: add standard-atmosphere wording, rerun final verification, and submit through reviewer.

## 2026-08-17 - Step 6: Public atmosphere wording

- Changed: `source/vcb_ballistics.h`; marked the conventional G1 coefficient as a standard-atmosphere value in the profile contract.
- Why: prevent integration callers from treating the solver as if it already applies live density, temperature, pressure, or altitude corrections.
- Verified: the recreated header differs from its retained `/tmp/vcb_ballistics.h.pre-standard-atmosphere-comment` backup only by the intended one-line contract comment.
- Still open: rerun strict compile and sanitizer checks with the final header, inspect scope, and submit through reviewer.

## 2026-08-17 - Step 7: Final core verification

- Changed: no repository code; reran the final strict compile and both sanitizer-backed harnesses after the public-contract wording change.
- Why: close the core role only from the exact final source/header state.
- Verified: strict C99 compile again produced zero warnings; focused behavior tests passed with the same numeric outputs; the 375-case envelope sweep again reported 328 solutions, 47 expected `no_solution` results, zero numeric errors, and no ASan/UBSan findings.
- Scope: core-owned repository files are `source/vcb_ballistics.c`, `source/vcb_ballistics.h`, and `docs/agents/ballistics-core.md`; no UI, I/O, persistence, build-generated file, submodule, or unrelated dirty file was changed.
- Still open: independent validator agreement, reviewer gate, target-toolchain compile/timing, and system integration are owned by subsequent phases.

## 2026-08-17 - Step 8: Reviewer contract and provenance fixes

- Changed: `source/vcb_ballistics.h`; documented that zero and requested range use the line-of-sight range axis and that impact speed is ground-relative.
- Changed: `docs/agents/ballistics-core.md`; recorded the failed reviewer gate and pinned the G1 retardation provenance.
- Why: remove frame ambiguity for integration callers and make the numerical model independently auditable.
- Review: FAIL because range-axis and impact-speed frames were only in handoff text and G1 coefficient provenance was not immutable; reviewer proposed the two documentation fixes applied here.
- Provenance: `https://github.com/grimwm/libballistics`, file `include/ballistics/drag.h`, immutable revision `161ec858933bcd3868369fb4198bfa9f959b8762`, permalink `https://github.com/grimwm/libballistics/blob/161ec858933bcd3868369fb4198bfa9f959b8762/include/ballistics/drag.h`, accessed 2026-08-17; that source file declares Apache License 2.0.
- Verified: `git ls-remote https://github.com/grimwm/libballistics.git refs/heads/master` resolved the inspected upstream to the recorded revision; the public field/function comments now state all requested frames in one line each.
- Still open: rerun strict compile and focused contract tests, log the results, and resubmit to reviewer.

## 2026-08-17 - Step 9: Review-fix verification

- Changed: no repository code; rebuilt and reran the sanitizer behavior and envelope harnesses after the reviewer-requested header fixes.
- Why: prove the contract-only changes did not alter solver behavior or introduce warnings.
- Verified: strict C99 compile passed with zero warnings; focused behavior tests reproduced the prior numeric results; the 375-case ASan/UBSan envelope sweep again produced 328 solutions, 47 expected `no_solution` results, and zero numeric or sanitizer failures.
- Still open: reviewer re-review, independent validator agreement, target-toolchain validation, and system integration.

## 2026-08-17 - Step 10: Reviewer gate passed

- Changed: no repository code; received and recorded the reviewer re-review verdict.
- Why: close the mandatory Phase 1 core review loop only after independent reproduction.
- Verified: reviewer strict C99 `-Werror` ASan/UBSan compile passed with zero warnings; its independently rebuilt 320-case harness passed with 166 solutions, 154 defined `no_solution` results, no findings, and 11.028 ms maximum-work host sanitizer timing.
- Review: PASS; both prior contract/provenance failures are closed and no further core changes were requested.
- Still open: validator compatibility, target compiler/timing/linked stack, and system integration remain later-phase gates.

## 2026-08-18 - Step 11: Phase 6 intake, baseline snapshot, G7 provenance

- Changed: `docs/agents/ballistics-core.md`; recorded the Phase 6 contract intake, the pre-edit baseline snapshot, and the pinned G7 source.
- Why: the G1 solver is reviewer-accepted and regression-locked, so the baseline must be frozen and the G7 numbers pinned before a single line of solver code changes.
- Baseline: copied the untracked pre-edit solver to `/tmp/vcb-ballistics-core-g7/baseline/vcb_ballistics.c` (md5 `2eceaffd40ddad1b039771ce5fdd90b4`) and `.../baseline/vcb_ballistics.h` (md5 `a8c3bbbcb8c0691499d234c4dd82e247`); `git show HEAD` cannot serve as the baseline because both files are untracked.
- Provenance: the already-pinned primary carries G7, so no alternative source was needed - `https://github.com/grimwm/libballistics`, file `include/ballistics/drag.h`, immutable revision `161ec858933bcd3868369fb4198bfa9f959b8762` (committed 2020-03-11T02:38:17Z), permalink `https://github.com/grimwm/libballistics/blob/161ec858933bcd3868369fb4198bfa9f959b8762/include/ballistics/drag.h`, accessed 2026-08-18; Apache License 2.0 header; `case G7:` at lines 115-125 gives nine segments.
- Verified: the downloaded copy is byte-authentic to that revision - local `git hash-object` gave blob `501d3b5125ebd498b0ecd2ed13a9ff2544e365bd`, and the GitHub contents API for the same path at that exact ref reported the identical blob sha and size 8400 (sha256 `339de4d9ce5bac3efc81a3479626c17c06e5c52b0f97f018b85807d9702fc762`, retained at `/tmp/vcb-ballistics-core-g7/drag.h`); the file's `case G1:` block matches the shipped `s_g1_segments` table, confirming the same transcription convention.
- Verified: an independent second copy of the identical G7 digits could not be indexed from here (grep.app returned a Vercel checkpoint page and the DuckDuckGo HTML endpoint returned no result links), so corroboration is numerical instead - segment-boundary continuity of the transcribed table plus the published G1-to-G7 retardation ratio, both measured in Step 14.
- Decision: internal names `s_g1_segments` and `vcb_ballistics_g1_drag_mps2(float, float)` are kept unchanged because `tests/ballistics-review/test_g1_thresholds.c` includes the .c and calls both; the family dispatch is added around them, not through them.
- Decision: with host GCC 13.3.0, `-fsanitize=address,undefined` does not instrument enum loads (checked with a 4-byte and an `-fshort-enums` 1-byte reproduction, and again with `-fsanitize=enum` explicitly added), so the required out-of-enum family rejection tests can run fully sanitized without the harness tripping on its own premise.
- Still open: implement the header and family dispatch, update the single production call site, then prove G1 bit-identity, G7 behavior, convergence, and rejection.

## 2026-08-18 - Step 12: Drag-family contract and G7 table

- Changed: `source/vcb_ballistics.h`; added `vcb_ballistics_drag_family_e` (`_g1 = 0`, `_g7 = 1`), renamed `g1_ballistic_coefficient` to `ballistic_coefficient` with the family-relative standard-atmosphere comment, and appended `drag_family` after `zero_range_m` so a zero-initialized profile is G1.
- Changed: `source/vcb_ballistics.c`; transcribed the nine-segment G7 table, generalized the segment lookup, dispatched on family, threaded the family through RK4, and rejected any non-enumerator family as `invalid_argument`.
- Changed: `lvgl_support/vcb_firing_solution.c`; one mechanical call-site update inside `firing_solution_recompute()` - `profile.g1_ballistic_coefficient` became `profile.ballistic_coefficient` and `profile.drag_family = vcb_ballistics_drag_family_g1;` was added; nothing else in that file was touched and the SD key it reads is still `g1_bc`, whose family plumbing belongs to integration.
- Why: keep the tree buildable for the roles working in parallel while making the family explicit at every call site, per the Phase 6 contract.
- Scope call-out for the system engineer: the bound macros were renamed `VCB_BALLISTICS_MIN_G1_BC`/`MAX_G1_BC` to `VCB_BALLISTICS_MIN_BC`/`MAX_BC` because the values are the shared 0.05-2.0 bounds for both families, and a `G1` name on a bound applied to a G7 BC is the same frame trap the field rename removes; a repository-wide grep found zero users of either macro outside `source/vcb_ballistics.[ch]`, so this breaks nothing and reverts in two lines if the contract owner prefers the old names.
- Decision: the family check sits with the `invalid_argument` group, before the range checks, so an invalid family outranks an out-of-range scalar; `vcb_ballistics_solve()` still `memset`s the output before any validation, so every rejection returns cleared output through the existing path.
- Decision: only the retardation table is family-dependent - RK4, the calm zero bisection, wind handling, bounds, statuses, and every guard are shared and untouched.
- Decision: `s_g1_segments` and `vcb_ballistics_g1_drag_mps2(float, float)` keep their names and signature (the shared lookup is factored out beneath them) so `tests/ballistics-review/test_g1_thresholds.c` keeps compiling; only the private struct tag became `vcb_ballistics_drag_segment_t`, which no test references.
- Transcription: G7 segments (threshold fps, coefficient, exponent) are 4200/1.29081656775919e-09/3.24121295355962, 3000/0.0171422231434847/1.27907168025204, 1470/2.33355948302505e-03/1.52693913274526, 1260/7.97592111627665e-04/1.67688974440324, 1110/5.71086414289273e-12/4.3212826264889, 960/3.02865108244904e-17/5.99074203776707, 670/7.52285155782535e-06/2.1738019851075, 540/1.31766281225189e-05/2.08774690257991, 0/1.34504843776525e-05/2.08702306738884.
- Verified: `/tmp/vcb-ballistics-core-g7/check_transcription.py` parsed both the pinned `drag.h` and the solver and compared every field as a double - G7 9/9 segments EXACT and G1 41/41 segments EXACT, with the only differences being cosmetic digit strings for identical values (integer thresholds written `4200.0f`, and the pre-existing G1 `5.71117468873424e-05` written `5.711174688734240e-05`).
- Verified: strict gate `gcc -std=c99 -Wall -Wextra -Werror -Wconversion -Wdouble-promotion -Wshadow -pedantic -c source/vcb_ballistics.c` produced zero warnings, and the same command with `-fshort-enums` added (target ABI) was also clean.
- Still open: G1 bit-identity against the frozen baseline, G7 behavior and physical sense, G7 step convergence, family rejection, and sanitizer runs.

## 2026-08-18 - Step 13: G1 bit-identity regression against the frozen baseline

- Changed: no repository code; built `/tmp/vcb-ballistics-core-g7/grid.c` against the frozen baseline solver and against the new solver and compared raw output bits.
- Why: the reviewer-accepted G1 behavior is regression-locked, so "unchanged" has to mean bit-identical, not close.
- Method: one harness source, `-DVCB_BASELINE` selecting `g1_ballistic_coefficient` with no family field and the default build selecting `ballistic_coefficient` plus an explicit `vcb_ballistics_drag_family_g1`; each case writes the `int` status and the six output floats as raw `unsigned int` bit patterns to a binary file, compared with `cmp`.
- Grid (375 cases): profiles (m/s, BC, sight m, zero m) 800/0.25/0.07/100, 900/0.10/0.05/100, 1500/2.00/0.20/1000, 50/0.05/0.00/10, 840/0.40/0.07/200; ranges 1, 100, 500, 800, 2000 m; wind speeds 0, 10, 40 m/s; directions 0, 45, 90, 180, 270 deg.
- Commands: `gcc -std=c99 -O2 -Wall -Wextra -Werror -Wconversion -Wdouble-promotion -Wshadow -pedantic [-DVCB_BASELINE] -I<src> grid.c <src>/vcb_ballistics.c -lm -o <bin>`; sanitized variants used `-std=c99 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer` with the same warning set, `UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1`, `ASAN_OPTIONS=detect_leaks=1`.
- Results: baseline and new-G1 both reported 375 cases, 330 ok, 45 `no_solution`, 0 other, 0 non-finite; `cmp` reported IDENTICAL at `-O2`, at `-O0`, under ASan/UBSan, and under ASan/UBSan with `-fshort-enums`; all four G1 dumps share md5 `e7e8f0440b46b1ebf4a11b6236e6d11a`, so every status and all 2,250 output floats are bit-identical and optimization-invariant.
- Results: the same grid under G7 gave 375 cases, 350 ok, 25 `no_solution`, 0 other, 0 non-finite (md5 `d7c3e94515d3eade5819827dde19145f`), identical across `-O2`, sanitized, and `-fshort-enums` sanitized builds; the 20 formerly unreachable cases now solve because G7 retains more velocity, which is the expected direction.
- Verified: every `no_solution` case in all runs returned fully zeroed output (checked per case, not sampled); zero ASan/UBSan findings in any run.
- Still open: G7 physical-sense numbers, table sanity, family rejection, and G7 step convergence.

## 2026-08-18 - Step 14: G7 table sanity, physical sense, and family rejection

- Changed: no repository code; built `/tmp/vcb-ballistics-core-g7/g7_checks.c`, which includes the solver translation unit directly (the same technique `tests/ballistics-review/test_g1_thresholds.c` uses) so it can reach the static tables.
- Command: `gcc -std=c99 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Wall -Wextra -Werror -Wconversion -Wdouble-promotion -Wshadow -pedantic -I<src> g7_checks.c -lm`; run with `UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1 ASAN_OPTIONS=detect_leaks=1`.
- Verified: 100 checks, 0 failures, zero ASan/UBSan findings; the same harness at `-O2 -fshort-enums` also gave 100/0, with `sizeof(vcb_ballistics_drag_family_e)` 4 on the host and 1 under `-fshort-enums` while `sizeof(vcb_ballistics_profile_t)` stayed 20 bytes in both.
- Transcription (second, independent check): the harness carries its own re-typed double copy of the nine upstream segments; over 1..5000 m/s the shipped float table deviates from that double reference by at most 1.895e-06 relative, at 368 m/s - float rounding only, no digit disagreement.
- Physical sense (BC 0.25, sight 0.07 m, zero 100 m, calm): at 800 m and 800 m/s muzzle, G1 gives drop 12.7841 m, TOF 1.972439 s, impact 266.839 m/s, elevation 15.9788 mrad, while G7 gives drop 6.7772 m, TOF 1.416184 s, impact 399.446 m/s, elevation 8.4713 mrad - G7 drops 6.0069 m less, 47.0 percent less drop, as the lower-drag reference projectile requires.
- Physical sense (same profile, other points): 700 m/s at 800 m gives 16.4248 m versus 9.4135 m (delta 7.0113 m); 900 m/s at 800 m gives 9.9368 m versus 5.0601 m (delta 4.8767 m); 800 m/s at 500 m gives 2.8572 m versus 1.8418 m; 800 m/s at 300 m gives 0.5736 m versus 0.4258 m; at every range from 300 m up, G7 also kept more velocity and shorter flight time, and at the 100 m zero both families returned drop within 1e-4 m of zero.
- Physical sense (wind): 800 m with a 10 m/s right crosswind gave G1 drift 9.7254 m, windage -12.1562 mrad and G7 drift 4.1621 m, windage -5.2025 mrad - same sign contract, less drift for the lower-drag family, no wind-handling change.
- Retardation ratio: G7 over G1 at equal BC is 0.6058 at 400 m/s, 0.5225 at 500, 0.5034 at 600, 0.5000 at 700, 0.4980 at 800, 0.4917 at 900, 0.4744 at 1000 m/s - the near-half ratio through the supersonic band is the published G1-to-G7 BC relationship for the same projectile and is the numerical corroboration standing in for the second source copy I could not index.
- Inherited-source call-out for the validator and the auditor: the pinned G7 fit is discontinuous at its knots and much coarser than the pinned G1 fit - measured relative jumps across the boundary are +3.101 percent at 4200 fps, +0.996 at 3000, +1.953 at 1470, +11.541 at 1260, +55.546 at 1110 fps (338.3 m/s, just under Mach 1), +2.642 at 960, +0.009 at 670, +1.548 at 540, against a worst G1 jump of 1.900 percent at 250 fps; nine G7 segments versus forty-one G1 segments. Every float jump reproduces the upstream double jump to within 1e-4 relative plus 1e-5, so this is the source table's property, not a transcription error - but any G7 trajectory decelerating through 338 m/s crosses a 55 percent retardation step, and that is the first thing to check if G7 long-range convergence or validator agreement looks worse than G1.
- Verified (rejection): raw family bytes 0x00000002, 0x00000007, 0x000000FF, 0x7FFFFFFF, and 0xFFFFFFFF (written through `memcpy`, all of them still non-enumerator values after 1-byte truncation) each returned `vcb_ballistics_status_invalid_argument` with all six outputs cleared to exactly 0.0f from a pre-dirtied solution struct; an invalid family combined with out-of-range muzzle velocity and BC also returned `invalid_argument`, confirming the family check precedes the range checks.
- Verified (shared bounds): BC 0.05 and 2.0 were accepted and BC 0.0499 and 2.001 returned `out_of_range` with cleared output, identically for `_g1` and `_g7`.
- Still open: G7 step convergence for a nominal and a severe case, then final re-verification of the exact submitted state.

## 2026-08-18 - Step 15: G7 step convergence and the call-site check

- Changed: no repository code; built `/tmp/vcb-ballistics-core-g7/converge.c` three times with `-DVCB_BALLISTICS_STEP_M=0.5f`, `0.25f`, and `0.125f`, all under ASan/UBSan and the strict warning set.
- Cases: g7-nominal 800 m/s, BC 0.25, sight 0.07 m, zero 100 m, 500 m, 10 m/s at 90 deg; g7-severe 900 m/s, BC 0.10, sight 0.05 m, zero 100 m, 800 m, 40 m/s at 90 deg; g7-knot-crossing 800 m/s, BC 0.25, 1500 m, 10 m/s at 90 deg, added deliberately because its impact speed decelerates through the 338 m/s discontinuity; each case repeated under G1 for scale. All 18 runs returned `ok`.
- Results (0.5 m versus 0.125 m absolute deltas): g7-nominal 0.000000417 s TOF, 0.000488 m/s impact, 0.000002146 m drop, 0.000001669 m drift, 0.000004053 mrad elevation, 0.000003338 mrad windage; g7-severe 0.000057220 s, 0.009186 m/s, 0.000642776 m, 0.002155304 m, 0.000802993 mrad, 0.002689361 mrad; g7-knot-crossing 0.000049353 s, 0.007477 m/s, 0.000820160 m, 0.000553131 m, 0.000547409 mrad, 0.000369072 mrad.
- Results (same deltas under G1, for scale): g1-nominal 0.000003338 m drop and 0.000003576 m drift; g1-severe 0.000095367 m drop and 0.000244141 m drift; g1-knot-crossing 0.000007629 m drop and 0.000003815 m drift; these reproduce the Step 4 severe G1 figures to the same order (0.5 m step error stays a fraction of a millimetre).
- Reading: the worst G7 production-step error anywhere is 0.64 mm of drop and 2.2 mm of drift on the severe 800 m case, about 4.6e-05 relative, roughly seven times the matching G1 error but three orders below any aiming-relevant quantity; the deliberate knot-crossing case shows no anomaly (0.82 mm drop), so the 55 percent retardation step at 338 m/s does not destabilise the fixed-step RK4 at 0.5 m.
- Honesty note on order: the nominal cases sit on the float rounding floor (some 0.25-to-0.125 deltas exceed the 0.5-to-0.125 deltas), so these numbers bound the error but cannot measure convergence order; the severe G7 drop ratio 0.000567/0.000075 is about 7.5, near third order in this noise. A real order measurement needs a double-precision reference and belongs to the ballistics-review empirical-order work, not here.
- Verified (call site): `gcc -fsyntax-only` over `lvgl_support/vcb_firing_solution.c` with the sim's include set exited 0; its only diagnostic is the pre-existing `portGET_RUN_TIME_COUNTER_VALUE` redefinition, which appears identically on untouched `lvgl_support/vcb_lrf_indicator.c` and comes from my ad-hoc include order, not from this change.
- Still open: final re-verification of the exact submitted state and the handoff summary.

## 2026-08-18 - Step 16: Final Phase 6 verification and handoff

- Changed: no repository code; reran every gate from the exact submitted source state, in one pass, with all build artifacts under `/tmp/vcb-ballistics-core-g7`.
- Verified (1) strict compile: `-std=c99 -Wall -Wextra -Werror -Wconversion -Wdouble-promotion -Wshadow -pedantic` on `source/vcb_ballistics.c` produced zero warnings, and again with `-fshort-enums` added.
- Verified (2) G1 bit-identity: the 375-case grid dumped from the frozen baseline and from the new solver at `-O2`, under ASan/UBSan, and under ASan/UBSan with `-fshort-enums` all compared IDENTICAL, md5 `e7e8f0440b46b1ebf4a11b6236e6d11a`; the G7 dump is md5 `d7c3e94515d3eade5819827dde19145f`; counts 330 ok / 45 no_solution for G1 and 350 / 25 for G7 in every build.
- Verified (3) G7 checks: 100 checks, 0 failures, zero sanitizer findings, reproduced under `-O2 -fshort-enums`.
- Verified (4) convergence: all three step builds reproduced their earlier CSVs byte-for-byte.
- Verified (5) other roles' G1 audit still runs: `tests/ballistics-review/test_g1_thresholds.c` compiled unchanged against the new solver and reported 238 checks, 0 failures, 36 exact thresholds, worst G1 fitted boundary jump 0.0193728749 at 250 fps - the same value my harness measured independently.
- Verified (6) transcription: the automated upstream comparison re-ran EXACT for G7 (9/9) and G1 (41/41).
- Resource delta (`-Os -fshort-enums`, identical flags both sides): text 3802 to 3998 bytes, plus 196 bytes for the nine-segment table and the dispatch; data and BSS stay 0; `vcb_ballistics_solve` remains the only exported symbol; static stack peaks unchanged at 240 bytes for `vcb_ballistics_integrate`, 128 for `vcb_ballistics_solve`, 64 for the derivative, with the new shared lookup a 32-byte leaf.
- Footprint: `source/vcb_ballistics.h`, `source/vcb_ballistics.c`, `docs/agents/ballistics-core.md`, and the two authorized lines in `lvgl_support/vcb_firing_solution.c`; no test file, no submodule file, no build-generated file, and no git operation of any kind.
- Call sites found referencing the old field name, left to their owners: `tests/ballistics/test_vcb_ballistics.c` (lines 34, 126, 249, 305, 306, 654) and `tests/ballistics-review/test_ballistics_review.c` (line 109); `tests/ballistics-review/test_g1_thresholds.c`, `test_zero_bisection.c`, and `benchmark_ballistics.c` do not use the field and still build. `vcb-gui-sim/main/inc/vcb_ballistics.h` is a one-line forwarder to the shared header and needs no change. `source/vcb_profiles.[ch]` and `lvgl_support/vcb_gui_cb.c` only carry the SD key and default named `g1_bc`, which is integration's to rename or map when the `drag_family` profile key lands.
- Still open for other roles: validator agreement on G7 reference data; ballistics-review audit of the nine transcribed segments and its empirical-order work; system-engineer confirmation of the `VCB_BALLISTICS_MIN_BC`/`MAX_BC` macro rename; integration to plumb the `"drag_family"` profile key (`g1`/`g7`, absent means g1) and decide whether the stored `g1_bc` key is renamed; target-toolchain compile and on-target timing of the G7 path.
- Judgement to carry forward: the pinned G7 fit is nine coarse segments with a 55.5 percent retardation discontinuity at 1110 fps (338.3 m/s), against forty-one G1 segments with a 1.9 percent worst jump. It is faithfully transcribed and it converges fine at the 0.5 m production step, but it is a lower-fidelity model than the G1 table this project already trusts, and G7 accuracy claims to the operator should be no stronger than that.

