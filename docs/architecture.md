# Architecture

## Layers and direction

```text
CLI / examples / tests
        |
writer + simulation orchestration
        |
registries + concrete integrators + concrete force models + stop conditions
        |
public interfaces + typed equation implementations
        |
model structures + vector math + status
        |
minimal selected platform port
```

Includes and calls flow downward. The CLI links `ballistics_core`; the library never links the CLI or Unity. Integrators know only arrays and a derivative callback. Dynamics knows force interfaces, not gravity/drag implementations. Equations know neither dynamics nor registries.

## Equation versus force model

An equation is a pure typed calculation with common metadata and a size-checked generic adapter. Air-relative velocity, aerodynamic drag, and force-to-acceleration are equations.

A force model participates in dynamics. It receives projectile and sampled environment data and contributes force. Basic drag calls two typed equations. New calculations do not require engine edits; new physical contributions do not require integrator edits.

## Construction and ownership

Create functions allocate through the port and have matching destroy functions. Simulation borrows dynamics, integrator, and stop conditions. Dynamics borrows projectile, environment, the force pointer array, and its models. CSV writer borrows a copied sink descriptor and never owns its context. Results own a geometrically growing sample array; no allocation occurs per integration step.

Applications destroy in reverse construction order. Distinct object graphs can run concurrently. A single mutable registry/simulation/result/writer is not concurrently safe.

## Simulation flow

```text
state
  -> integrator step
       -> generic dynamics derivative (one or four calls)
            -> environment sample
            -> each force model in registration order
            -> force sum / mass
  -> evaluate all stop interfaces
  -> choose fixed priority: invalid, ground, distance, time
  -> emit ordinary or refined final sample
```

Ground crossing brackets the configured plane and linearly interpolates time, position, and velocity. The interpolated impact replaces the below-ground grid state.

## Registries

Four fixed-capacity registry families store borrowed stable identifiers and uniform family factories. Registration rejects invalid identifiers and duplicates. Lookup, enumeration, and instance creation are public. Built-ins are statically linked and registered in `src/registry/builtin_registrations.c`; Phase One performs no dynamic loading.

## Port and byte-sink flow

The selected port exposes monotonic time, allocation/deallocation, debug bytes, and reserved mutex hooks. Linux selects `linux_port.c`; unknown targets select `unsupported_port.c`. There is no file API.

```text
CLI FILE* -> application byte-sink callbacks -> CSV writer -> bytes
embedded UART/SD callback --------------------^ (future application)
```

## Determinism and diagnostics

Force iteration, condition priority, samples, columns, formatting (`%.9e`), and newlines are fixed. The CLI selects the C numeric locale. Debug output uses a separate global diagnostic configuration and port timestamp; it does not affect numerical state.

## Future plugin shape

Stable identifiers and uniform factories are compatible with a future loader, but loading, ABI negotiation, and trust policy are deferred. New enum values and interface versions are appended; existing values are not renumbered.
