# Session Resume

Read [`docs/interface_contracts.md`](docs/interface_contracts.md), [`docs/adr.md`](docs/adr.md), and [`docs/implementation_plan.md`](docs/implementation_plan.md) before changing code.

Coordinates: right-handed `+x` downrange, `+y` right, `+z` upward; gravity is normally negative `z`.

Units: SI only—metres, seconds, kilograms, kelvin, pascals, radians; simulation arithmetic uses IEEE-754 `double`.

Current implementation stage: stage 12 complete; stage 13 is next.

Public-contract changes require a rationale, affected-module list, same-commit contract update, and affected-test reruns. No core module may access files. No unsafe floating-point compiler option is permitted.
