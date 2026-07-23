# Python Integration Plan

Phase One deliberately ships no Python module. The core is already suitable for one: public functions are C ABI, symbols use `BALLISTICS_API`, handles with significant mutable state are opaque, create/destroy is explicit, there is no application-global scenario state, and shared builds use `BALLISTICS_BUILD_SHARED`.

## Options

- `ctypes`: load `libballistics_core.so`, declare structures/signatures, and wrap every owned pointer in a Python context manager/finalizer.
- `cffi`: describe the public headers and build an ABI- or API-mode wrapper; this is likely the quickest ergonomic prototype.
- CPython extension: provide the best validation and buffer integration, but requires compiled binding code and CPython lifecycle/error handling.

A binding should expose high-level scenario/result objects, convert status codes to Python exceptions, copy trajectory samples into a Python/NumPy-owned buffer, release C objects deterministically, and hold the GIL policy explicitly. It must not expose private structs or let borrowed environment/model pointers outlive their owners.

Before freezing a cross-language ABI, add an API-version query, structure-size/version negotiation for ABI-visible value structs, shared-library symbol tests, and Python concurrency tests. Dynamic plugins remain a separate concern.
