# Shutdown

> Triggers a reaction once during system shutdown for cleanup.

## Syntax

```cpp
on<Shutdown>().then([] {
    // Runs once when the system is shutting down
});
```

## Behavior

`Shutdown` fires after `PowerPlant::shutdown()` is called.
At this point the thread pool is still active, allowing shutdown tasks to perform cleanup work such as closing connections, flushing buffers, or saving state.

```mermaid
graph LR
    A[Install Reactors] --> B["PowerPlant::start()"]
    B --> C[Startup reactions fire]
    C --> D[Normal execution]
    D --> E["PowerPlant::shutdown()"]
    E --> F[Shutdown reactions fire]
    F --> G[Threads joined]
```

After all Shutdown tasks complete, the thread pool threads are joined and the PowerPlant terminates.
Each Shutdown reaction runs exactly once per PowerPlant lifecycle.

## Example

```cpp
#include <nuclear>

class CleanupReactor : public NUClear::Reactor {
public:
    CleanupReactor(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {

        on<Shutdown>().then([this] {
            log<INFO>("System shutting down, cleaning up...");
            close_connections();
            flush_buffers();
        });
    }
};
```

## Notes

- Shutdown fires only once — it cannot re-trigger.
- The thread pool is still active during Shutdown, so tasks execute normally.
- **After shutdown begins, no new non-inline tasks can be submitted.** If a Shutdown reaction emits a message (non-inline), the triggered reactions will not execute.
    Only `emit<Scope::INLINE>` works during shutdown.
- Common uses: close connections, flush buffers, save state, log shutdown info.
- After all Shutdown reactions complete, threads are joined and the PowerPlant exits.

## See Also

- [Startup](startup.md) — trigger a reaction when the system starts
- [Once](once.md) — ensure any reaction runs exactly once
- [Lifecycle](../../explanation/lifecycle.md) — full PowerPlant lifecycle explanation
