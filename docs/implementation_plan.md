# Phase One Implementation Plan

The foundation contract commit precedes dependent code. Each stage ends with a clean build appropriate to repository state, an updated `CLAUDE.md` stage line, and its own git commit. Contract changes include rationale, affected modules/tests, same-commit documentation, and reruns.

## Stages

1. CMake skeleton: targets/options, target warnings, sanitizer/coverage modules, port selection, ignores, formatting, license.
2. Status codes and public types.
3. Port layer: Linux/unsupported selection, no file API.
4. Debug layer: level/callback/source/timestamp/compile-time controls.
5. Vector mathematics.
6. Model structures. **QA M1**: clean GCC/Clang where available, warnings, API/dependency/platform/log inspection.
7. Four registries with validation, lookup, enumeration, creation.
8. Equation framework and three typed built-ins.
9. Environment interface and constant implementation.
10. Force-model interface.
11. Constant gravity implementation.
12. Basic drag implementation calling equations.
13. Dynamics callback. **QA M2**: clean build and full available inspection.
14. Generic integrator interface.
15. Euler implementation.
16. Classical RK4 implementation.
17. Simulation orchestration and results.
18. Invalid/ground/distance/time stop conditions. **QA M3**: orchestration/numerical/ownership inspection.
19. Byte sink and deterministic CSV writer.
20. CLI with fictional profile and application-owned file sink.
21. Vendored Unity and required unit tests.
22. Required integration tests, including nonlinear measured order and interpolation.
23. ASan/UBSan run and coverage-config smoke check.
24. README, architecture/extension/port/Python docs, scripts, example.
25. Fresh Debug/test/sanitizer, Release/static, and shared builds; CLI validation. **QA M4**: independent full inspection and repair loop.

## Commit convention

Use `stage NN: description` and `fix: ... (QA Mx defect n)`. No co-author or tool-attribution trailers. Use existing git identity. Ignore build artifacts. Push milestones only to an existing `origin`; otherwise retain local commits and report why.

## QA defect loop

A fresh-context QA agent receives only repository path and milestone. It performs clean configuration/build/test/sanitizer work, dependency and prohibited-call searches, and precise defect reports. The lead commits fixes; QA reruns affected tests and then the complete available suite.

## Current progress

- Foundation contracts: complete.
- Stages 1–23: complete and committed.
- Stages 24–25: complete; documentation, examples, fresh Debug/sanitizer, Release/static, and shared acceptance builds passed.
- QA M1, M2, and M3: passed after documented repair loops.
- QA M4: final clean acceptance passed with no defects.

Phase Two native GUI work is governed by [`phase_two_plan.md`](phase_two_plan.md). The frozen Phase
One public C contracts remain unchanged.
Phase Two native GUI stages 1–6 are implemented and final acceptance passed; see `phase_two_acceptance.md`.
Phase 2.1 asset-backed visual upgrade passed; see `phase_2_1_acceptance.md`.
Phase 2.2 visual realism passed; see `phase_2_2_visual_realism.md` and `phase_2_2_acceptance.md`.
