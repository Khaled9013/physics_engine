# Adding an Integrator

1. Add a public typed config/create/factory header under `include/ballistics/integrators/`.
2. Add one implementation under `src/integrators/` with a `BallisticsIntegratorVTable`.
3. Integrate only the supplied `double` array by calling the supplied derivative callback. Do not include projectile or physics-model headers.
4. Validate exact factory `config_size`; document whether `NULL,0` has defaults.
5. Add one identifier/factory line to `ballistics_register_builtin_integrators`.
6. Add the source to the `ballistics_core` target and add unit/convergence tests.

The Phase One interface is fixed-step. A future adaptive implementation must use a versioned extension rather than changing v1 semantics. Preserve finite checks, propagate callback statuses, never enable relaxed floating-point flags, and allocate no scratch memory per step. `euler.v1` and `rk4.v1` are reference implementations.
