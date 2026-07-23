# Porting Guide

Phase One ships only Linux plus the required unknown-platform fallback. Do not add fictional Windows, Arduino, or STM32 stubs.

## Adding a real platform

1. Create `src/port/newplatform_port.c` only when the platform can be implemented and tested.
2. Implement every service declared by `ballistics_port.h`:
   - monotonic seconds,
   - allocate and deallocate,
   - debug byte output,
   - reserved mutex create/lock/unlock/destroy hooks.
3. Keep platform headers in that source file. Do not expose vendor/POSIX types publicly.
4. Add a branch to `cmake/PortSelection.cmake` that selects exactly this source for the real `CMAKE_SYSTEM_NAME`/toolchain. The final target must compile exactly one port source.
5. Build core unit tests for the target or provide a host-tested conformance harness. Verify allocation pairing, monotonic behavior, debug partial writes, and unsupported/error statuses.

Never add file calls to the port. The core has no file concept.

## Embedded byte sinks

A UART application supplies callbacks like:

```c
BallisticsStatus uart_write(void *ctx, const void *data, size_t size)
{
    return board_uart_transmit(ctx, data, size) ? BALLISTICS_STATUS_OK
                                                 : BALLISTICS_STATUS_IO_ERROR;
}
BallisticsByteSink sink = {uart_write, NULL, &uart_handle};
```

An SD-card application may hold its filesystem/file object in application context and implement `write`/`flush` there. The CSV writer receives the sink descriptor, never closes it, and needs no change. This keeps targets without desktop filesystems honest.

For no-heap targets, first add a versioned allocator strategy or caller-storage constructors; do not silently bypass the port contract.
