# Adding a Force Model

A force model contributes one real-world physical force to dynamics. It is distinct from a reusable equation. A model may call equations internally—`basic-drag.v1` calls air-relative velocity and aerodynamic-drag equations—but equations never call dynamics.

1. Define a typed config and direct create/factory API in a focused public header.
2. Store validated copied config in a private context.
3. Implement `BallisticsForceModelVTable.calculate_force` and `destroy` in one source file.
4. Keep calculation const and allocation-free. Return force in newtons.
5. Use typed equation functions for reusable formulas rather than duplicating them.
6. Add one stable versioned identifier/factory line to `ballistics_register_builtin_force_models`.
7. Add the source to CMake and unit/integration tests.

Factory rules: exact `config_size`, validate before copying, clear output first, and accept `NULL,0` only for documented defaults. The model owns only its object/context. Projectile, state, and sampled environment arguments are borrowed for the call.

Dynamics already enumerates every enabled `BallisticsForceModel`; do not edit `simulation.c`, dynamics, Euler, or RK4 when adding a normal model.
