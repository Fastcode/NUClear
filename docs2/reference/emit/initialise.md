# Scope::INITIALISE

> Queues data to be emitted during the system startup phase.

## Syntax

```cpp
emit<Scope::INITIALISE>(std::make_unique<T>(args...));
```

## Behavior

The behavior depends on when the emit is called relative to `PowerPlant::start()`:

**Before `PowerPlant::start()`** (during reactor installation):

1. A task is created that will perform a `Scope::LOCAL` emit of the data.
2. The task is submitted to the PowerPlant's task queue.
3. These tasks execute during the initialisation phase — after all reactors are installed but as part of the startup sequence.

**After `PowerPlant::start()`** (during normal execution):

- Behaves identically to `Scope::LOCAL`.

```mermaid
stateDiagram-v2
    [*] --> Installing : Reactors constructed
    Installing --> Initialising : PowerPlant::start()
    Initialising --> Running : Initialisation tasks complete

    state Installing {
        [*] --> Queue : emit&lt;Scope::INITIALISE&gt;
        Queue --> [*] : Tasks queued
    }

    state Initialising {
        [*] --> Execute : Queued tasks run
        Execute --> [*] : Local emit for each
    }
```

## Example

```cpp
#include <nuclear>

struct Config {
    int threshold;
    bool enabled;
};

class Defaults : public NUClear::Reactor {
public:
    explicit Defaults(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {

        // Set up default configuration available at startup
        emit<Scope::INITIALISE>(std::make_unique<Config>(Config{100, true}));

        on<Trigger<Config>>().then([this](const Config& cfg) {
            log<INFO>("Config loaded: threshold=", cfg.threshold);
        });
    }
};
```

## Notes

- Use this in reactor constructors to ensure data is available before the main execution phase.
- Reactions triggered by initialise emits run during the initialisation phase, before normal execution begins.
- Order of initialise emits across reactors depends on reactor installation order.

!!! tip "When do you need this?"
    `Scope::INITIALISE` is rarely required in practice. In most cases, emitting data in a `Startup` reaction achieves the same result more clearly. This scope exists specifically for resolving circular dependency situations where Reactor A needs data from Reactor B during construction, but Reactor B also needs to react to emissions from Reactor A. In general, prefer `Startup` unless you have a specific ordering constraint during the installation phase.

## See Also

- [Local](local.md) — the underlying emit mechanism used after the delay
- [Startup DSL word](../dsl/startup.md) — triggered when the system enters the running phase
