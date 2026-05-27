# LogLevel

> Enum defining severity levels for log messages.

## Overview

`LogLevel` is a class wrapping an enum that defines the severity of log messages. Each reactor has a `log_level` threshold controlling which messages are displayed and a `min_log_level` that is always emitted regardless.

## API

### Values

| Level   | Value | Description                                           |
| ------- | ----- | ----------------------------------------------------- |
| `TRACE` | 1     | Extremely detailed execution tracing                  |
| `DEBUG` | 2     | Diagnostic information; inputs/outputs of computation |
| `INFO`  | 3     | High-level operational messages                       |
| `WARN`  | 4     | Non-fatal issues requiring attention                  |
| `ERROR` | 5     | Unexpected behaviour; constraint violations           |
| `FATAL` | 6     | Unrecoverable system failure                          |

`UNKNOWN` (value 0) is internal-only; used when a log originates outside a reaction context.

### Reactor Log Control

| Member          | Default | Description                                   |
| --------------- | ------- | --------------------------------------------- |
| `log_level`     | `INFO`  | Messages at or above this level are displayed |
| `min_log_level` | `FATAL` | Always emitted regardless of `log_level`      |

If `log_level` is set below `min_log_level`, both thresholds apply (the lower of the two determines display).

## Example

```cpp
class MyReactor : public NUClear::Reactor {
public:
    explicit MyReactor(std::unique_ptr<NUClear::Environment> environment)
        : Reactor(std::move(environment)) {

        // Adjust this reactor's log threshold
        log_level = DEBUG;

        on<Startup>().then([this] {
            log<TRACE>("Very detailed trace");   // Not displayed (below DEBUG)
            log<DEBUG>("Diagnostic info");       // Displayed
            log<INFO>("System ready");           // Displayed
            log<WARN>("Low disk space");         // Displayed
            log<ERROR>("Connection failed");     // Displayed
            log<FATAL>("Unrecoverable error");   // Always displayed
        });
    }
};
```

## See Also

- [Reactor](reactor.md)
- [PowerPlant](power-plant.md)
