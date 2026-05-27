# Reactor

> Base class for all reactive components in a NUClear system.

## Overview

`Reactor` provides the interface for binding callbacks to events and emitting data.
All user-defined reactors inherit from this class.
DSL words are aliased as protected members so they can be used unqualified within reactor code.

## API

### Constructor

```cpp
explicit Reactor(std::unique_ptr<Environment> environment);
```

Called by `PowerPlant::install`.
Not invoked directly by user code.

### `on<DSLWords...>(args...).then(callback)`

```cpp
template <typename... DSL, typename... Arguments>
Binder on(Arguments&&... args);
```

Creates a reaction bound to the specified DSL words.
Chain with `.then(callback)` or `.then("label", callback)` to complete registration.
Returns a `ReactionHandle`.

```cpp
auto handle = on<Trigger<Message>>().then([](const Message& msg) {
    // handle msg
});

on<Every<1, std::chrono::seconds>>().then("ticker", [] {
    // runs every second
});
```

### `emit<Scope>(data)`

```cpp
template <template<typename> class... Scope, typename T, typename... Args>
void emit(std::unique_ptr<T>&& data, Args&&... args);
```

Emits data into the system.
Defaults to `Local` scope if no scope is specified.

```cpp
emit(std::make_unique<Message>("hello"));
emit<Scope::NETWORK>(std::make_unique<Message>("broadcast"));
```

### `log<Level>(args...)`

```cpp
template <LogLevel::Value level, typename... Arguments>
void log(Arguments&&... args);
```

Logs a message from this reactor.
The reactor's name is attached to the log output.

```cpp
log<INFO>("Processing", count, "items");
```

### Public Members

| Member          | Type                | Description                                     |
| --------------- | ------------------- | ----------------------------------------------- |
| `powerplant`    | `PowerPlant&`       | Reference to the owning PowerPlant              |
| `reactor_name`  | `const std::string` | Demangled type name of this reactor             |
| `log_level`     | `LogLevel`          | Display threshold (default: `INFO`)             |
| `min_log_level` | `LogLevel`          | Minimum always-emitted level (default: `FATAL`) |

### Protected Type Aliases

All DSL words are aliased so they can be used without namespace qualification:

| Alias                        | DSL Word                              |
| ---------------------------- | ------------------------------------- |
| `Trigger<T>`                 | Trigger on new data of type T         |
| `With<T>`                    | Require most recent data of type T    |
| `Every<ticks, period>`       | Periodic timer                        |
| `Startup`                    | Triggered once on PowerPlant start    |
| `Shutdown`                   | Triggered on PowerPlant shutdown      |
| `Always`                     | Continuously re-triggered             |
| `Once`                       | Trigger at most once                  |
| `IO`                         | File descriptor readiness             |
| `UDP`                        | UDP socket events                     |
| `TCP`                        | TCP socket events                     |
| `Network<T>`                 | Networked data                        |
| `Sync<T>`                    | Synchronization group                 |
| `Single`                     | At most one task at a time            |
| `Buffer<N>`                  | Allow up to N concurrent tasks        |
| `Pool<T>`                    | Execute on a named thread pool        |
| `Group<T>`                   | Concurrency group                     |
| `Priority`                   | Task priority                         |
| `MainThread`                 | Execute on the main thread            |
| `Inline`                     | Execute inline in the emitting thread |
| `Idle<T>`                    | Triggered when a pool is idle         |
| `Optional<T...>`             | Data may not be present               |
| `Last<N, T...>`              | Last N values                         |
| `Per<period>`                | Rate limiter                          |
| `Watchdog<T, ticks, period>` | Watchdog timer                        |
| `TaskScope<T>`               | Task-scoped data                      |

### LogLevel Constants

```cpp
static constexpr LogLevel TRACE = LogLevel::TRACE;
static constexpr LogLevel DEBUG = LogLevel::DEBUG;
static constexpr LogLevel INFO  = LogLevel::INFO;
static constexpr LogLevel WARN  = LogLevel::WARN;
static constexpr LogLevel ERROR = LogLevel::ERROR;
static constexpr LogLevel FATAL = LogLevel::FATAL;
```

### Emit Scopes

Accessed via the nested `Scope` struct:

| Scope               | Description                    |
| ------------------- | ------------------------------ |
| `Scope::LOCAL`      | Default; tasks via thread pool |
| `Scope::INLINE`     | Execute in the emitting thread |
| `Scope::DELAY`      | Delayed emission               |
| `Scope::INITIALIZE` | Available before startup       |
| `Scope::NETWORK`    | Broadcast over network         |
| `Scope::UDP`        | Emit via UDP                   |
| `Scope::WATCHDOG`   | Service a watchdog             |

## Example

```cpp
#include <nuclear>

class Sensor : public NUClear::Reactor {
public:
    explicit Sensor(std::unique_ptr<NUClear::Environment> environment)
        : Reactor(std::move(environment)) {

        on<Every<100, std::chrono::milliseconds>>().then("read_sensor", [this] {
            auto data = std::make_unique<SensorData>(read());
            emit(std::move(data));
        });

        on<Trigger<SensorData>>().then([this](const SensorData& d) {
            log<DEBUG>("Received sensor value:", d.value);
        });
    }
};
```

## See Also

- [PowerPlant](power-plant.md)
- [ReactionHandle](reaction-handle.md)
- [LogLevel](log-levels.md)
