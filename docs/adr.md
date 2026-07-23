# Architecture Decision Record

Status: accepted for Phase One, 2026-07-23.

## Context

Phase One is a virtual external-ballistics research environment with no physical weapon, sensor, or live-fire dependency. The core is C17, uses IEEE-754 `double`, and supports static or shared-library builds. Coordinates are right-handed: `+x` forward/downrange, `+y` right, `+z` upward. Internal units are SI.

## ADR-001: layered, inward-facing dependencies

```text
application / examples / tests
             |
output + simulation orchestration
             |
registries + integrators + force-model implementations
             |
interface/API layer + equation implementations
             |
models + mathematics + core utilities
             |
platform abstraction
```

Higher layers may use lower layers. Lower layers do not include higher-layer headers. `ballistics_core` never depends on the CLI, Unity, Python, a GUI, or a game engine. Concrete implementations are selected through interfaces or registries and are never hard-coded into integrators.

## ADR-002: equations and force models are distinct

An equation is a reusable calculation with metadata and no simulation-loop knowledge. Typed entry points preserve compile-time checking; a size-checked common adapter supports registry-created objects. A force model is a dynamics participant that contributes physical force. It may call equations, but equations never accumulate force or invoke the engine. The simulation enumerates `BallisticsForceModel` objects only.

## ADR-003: registries are static now and plugin-ready later

Equation, force-model, integrator, and writer registries provide registration, lookup, duplicate rejection, enumeration, and creation by stable versioned identifier. Built-ins are centralized. Dynamic loading is absent in Phase One.

Factories use `(const void *config, size_t config_size, **out)`. Every adapter validates exact size and values; every implementation also exposes typed creation. `NULL, 0` requests documented defaults only where supported.

## ADR-004: core has no filesystem abstraction

Writers receive a `BallisticsByteSink`; they write and flush but never open, name, close, or free the context. The Linux CLI alone constructs a `FILE *` sink. Tests use a memory sink. Embedded applications can use UART or SD-card sinks without core changes. The port layer contains only monotonic time, memory, debug bytes, and reserved mutex hooks.

## ADR-005: fixed-step integration

Euler and classical RK4 use a generic derivative callback over a contiguous `double` vector. Phase One chooses fixed steps and shortens the last step to reach maximum time exactly. The interface remains versionable for adaptive methods. No configuration enables relaxed floating-point semantics; Release uses `-O2`.

## ADR-006: ground crossing refinement

The ground condition detects a bracket and computes:

```text
alpha = (ground_z - previous_z) / (current_z - previous_z)
t_impact = t_previous + alpha * (t_current - t_previous)
```

It linearly interpolates every position and velocity component. This is deterministic and far more accurate than grid snapping, but is not a dense-output RK root solve; for smooth trajectories its time error is normally second-order.

## ADR-007: ownership and allocation

Opaque objects use create/destroy pairs and allocate through the port. Configuration is copied when lifetime would be ambiguous. Simulation borrows dynamics, integrator, and stop conditions. Dynamics borrows projectile data, environment, and force models. Results own controlled-growth sample storage. No per-step allocation occurs.

## ADR-008: deterministic CSV

CSV uses fixed columns, `%.9e` for every float, fixed commas/newlines, and insertion-order samples. The CLI selects the C numeric locale. Identical configurations in the same binary produce byte-identical bytes.

## ADR-009: centralized debug

Library modules do not call `printf`, `fprintf`, or platform clocks. Logs carry level, subsystem, source, function, line, and port timestamp. One documented process-wide debug configuration controls callback and level; it does not affect simulation results. Release can compile out DEBUG/TRACE.

## ADR-010: native PyQt/Panda3D GUI stays outside the core

Phase Two uses PyQt6 for a native Linux desktop shell and embeds Panda3D as the GPU-rendered range
viewport. There is no browser or network service. Shot calculations execute in a Qt worker pool by
invoking the existing C CLI without a shell; the GUI owns and removes temporary CSV files.

Qt and Panda3D are application dependencies only. The Phase One core retains no Python, GUI,
rendering, input, game-engine, or filesystem dependency. The initial embedded-window port targets
Linux/X11. Scene graph access remains on the GUI thread while immutable jobs run in workers.

## Consequences

The design uses more small objects than a monolith, but dependencies, ownership, testing, future plugins, Python bindings, and embedded ports remain explicit. Phase One deliberately omits dynamic loading, JSON profiles, adaptive steps, and Python bindings.
