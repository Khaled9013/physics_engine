# Phase Two Native GUI Acceptance

Date: 2026-07-23.

Phase 2.1 supersedes the initial presentation findings below. See
[`phase_2_1_acceptance.md`](phase_2_1_acceptance.md) for the asset-backed visual acceptance.

## Verified host

- Linux/X11 (`xcb` Qt backend).
- GCC 13.3.0.
- Python 3.14.3.
- PyQt6 6.11.0 with Qt 6.11.1.
- Panda3D 1.10.16 using `glxGraphicsPipe`.
- NVIDIA GeForce RTX 3050, driver 595.71.05.

Clang was not installed on the acceptance host. The code retains the target-scoped GCC/Clang warning
configuration from Phase One, but this Phase Two pass does not claim a fresh Clang build.

## Build and test results

- Fresh GCC Debug static build: passed with strict warnings and no warnings emitted.
- Fresh GCC Release static build: passed; Release CLI scenario passed.
- Fresh GCC Release shared build: passed; dynamically linked CLI scenario passed.
- Fresh GCC ASan/UBSan build: all three CTest targets passed with leak detection and halt-on-error.
- CTest: unit, analytical integration, and CLI-option suites passed, 3/3.
- Python: validation, deterministic CLI bridge, coordinates, target interpolation/scoring, and real
  `QThreadPool` worker passed, 13/13.
- Native GPU smoke: a real C shot rendered through the embedded Panda3D GLX child; hip and circular
  scope framebuffers and a complete PyQt-window capture were inspected.
- Setup and launch helpers: `setup_gui.sh`, `run_gui.sh`, and `run_gui_tests.sh --smoke` passed.
- Network boundary search: no web server, web view, socket listener, or HTTP framework exists in the
  GUI application.

## Preserved Phase One guarantees

The C core public contract was not changed. The core still contains no Python, Qt, Panda3D, game
engine, filesystem, or GUI dependency. Deterministic CSV remains authoritative and two identical
GUI-bridge configurations were byte-identical.

## Known limitations

- The embedded Panda3D window currently targets Linux/X11 or XWayland through Qt's `xcb` backend;
  native Wayland embedding is not implemented.
- The initial procedural presentation was replaced in Phase 2.1; the upgraded result remains stylized rather than photoreal.
- Python calls the C CLI in a worker process; direct shared-library bindings remain future work.
- The target score uses linear interpolation at a rendered plane and is presentation feedback, not
  an inverse aiming solution.
- There is no audio, asset editor, profile persistence, 6-DOF model, terrain, or multiplayer.
