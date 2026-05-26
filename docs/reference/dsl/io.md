# IO

Triggers a reaction when events occur on a file descriptor.

## Syntax

```cpp
on<IO>(fd, events).then([](const IO::Event& event) { ... });
```

## Parameters

| Parameter | Type            | Description                          |
|-----------|-----------------|--------------------------------------|
| `fd`      | `fd_t`          | The file descriptor to monitor       |
| `events`  | `IO::EventType` | Bitmask of events to watch (OR'd)    |

## Events

| Event       | Description                                      |
|-------------|--------------------------------------------------|
| `IO::READ`  | Data is available to read on the file descriptor |
| `IO::WRITE` | The file descriptor is ready for writing         |
| `IO::CLOSE` | The file descriptor has been closed              |
| `IO::ERROR` | An error occurred on the file descriptor         |

Events can be combined with bitwise OR:

```cpp
on<IO>(fd, IO::READ | IO::WRITE)
```

## Behavior

- Monitors the specified file descriptor for the requested events.
- When any watched event occurs, the reaction fires with an `IO::Event` describing what happened.
- The `IO::Event` struct contains:
  - `fd` — the file descriptor that triggered the event.
  - `events` — a bitmask indicating which events occurred.
- Multiple events may be reported simultaneously in a single callback.
- Works with any file descriptor: sockets, pipes, files, serial ports.
- Managed by the IOController extension (epoll on Linux, kqueue on macOS, a separate implementation on Windows).

## Example

```cpp
on<IO>(my_fd, IO::READ).then([](const IO::Event& event) {
    if (event.events & IO::READ) {
        char buf[1024];
        ssize_t n = read(event.fd, buf, sizeof(buf));
        // process data
    }
    if (event.events & IO::CLOSE) {
        // fd was closed
    }
});
```

## Notes

- The `IO::CLOSE` event may be delivered alongside other events in the same callback.
- Platform-specific backends are transparent to the user; the API is identical across Linux, macOS, and Windows.
- On Windows, only socket file descriptors are supported.

## See Also

- [TCP](tcp.md)
- [UDP](udp.md)
- [Monitoring IO Events](../../how-to/io-events.md)
