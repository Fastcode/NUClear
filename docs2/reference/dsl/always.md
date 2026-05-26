# Always

Runs a reaction continuously in a dedicated thread. When the callback returns, it immediately executes again. Replacement for infinite `while(true)` loops.

## Syntax

```cpp
on<Always>().then([] {
    // Called repeatedly in a dedicated thread
});
```

## Behavior

Always registers an idle task that continuously re-triggers its reaction. It runs in its own dedicated thread pool with a concurrency of 1, separate from the default pool. Only one instance of the reaction executes at a time.

```mermaid
graph LR
    A[Reaction starts] --> B[Callback executes]
    B --> C[Callback returns]
    C --> A
```

The execution loop runs indefinitely until the PowerPlant shuts down.

## Example

```cpp
#include <nuclear>

class HardwarePoller : public NUClear::Reactor {
public:
    HardwarePoller(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {

        on<Always>().then([this] {
            auto data = read_sensor();  // Blocking I/O
            emit(std::make_unique<SensorData>(data));
        });
    }
};
```

## Notes

!!! warning "CPU Spinning"
    If the callback returns without blocking or sleeping, the reaction will busy-spin and consume 100% CPU on its dedicated thread. Always ensure the callback performs blocking work or includes an explicit sleep/wait.

!!! warning "Don't block forever"
    The callback must periodically return or yield so that NUClear can shut down the system gracefully. If you block indefinitely (e.g., an infinite wait with no timeout), the PowerPlant cannot stop the thread when shutdown is requested. Use timeouts on blocking calls and check if the system is shutting down regularly.

- Always gets its own thread — it does not consume a thread from the default pool.
- Good for: polling hardware, running event loops, blocking I/O loops.
- Bad for: work that could be event-driven. Prefer [Trigger](trigger.md) or [Every](every.md) instead.

## See Also

- [Every](every.md) — periodic execution at a fixed interval
- [Idle](idle.md) — runs opportunistically when the thread pool has no other work
- [Pool](pool.md) — configure custom thread pools
