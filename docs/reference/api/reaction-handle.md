# ReactionHandle

> Controls a reaction's lifecycle after creation.

## Overview

Returned by `on<...>().then(...)`, a `ReactionHandle` allows enabling, disabling, and permanently removing a reaction at runtime.
Disabling a reaction prevents new tasks from being generated when triggered; enabling resumes normal operation.

## API

### `enable()`

```cpp
ReactionHandle& enable();
```

Enables the reaction.
Triggered events will generate tasks again.

### `disable()`

```cpp
ReactionHandle& disable();
```

Disables the reaction.
No tasks will be created when triggered.
Configuration is preserved.

### `enable(bool set)`

```cpp
ReactionHandle& enable(const bool& set);
```

Sets enabled state.
`true` to enable, `false` to disable.

### `enabled() const`

```cpp
bool enabled() const;
```

Returns `true` if the reaction is currently enabled.

### `unbind()`

```cpp
void unbind();
```

Permanently removes the reaction from the system.
This is irreversible.

### `operator bool()`

```cpp
operator bool() const;
```

Returns `true` if the handle holds a valid reaction (which may or may not still be bound).

## Example

```cpp
class Controller : public NUClear::Reactor {
public:
    explicit Controller(std::unique_ptr<NUClear::Environment> environment)
        : Reactor(std::move(environment)) {

        auto handle = on<Trigger<SensorData>>().then([](const SensorData& d) {
            process(d);
        });

        on<Trigger<Pause>>().then([handle]() mutable {
            handle.disable();   // Temporarily stop processing
        });

        on<Trigger<Resume>>().then([handle]() mutable {
            handle.enable();    // Resume processing
        });

        on<Trigger<Cleanup>>().then([handle]() mutable {
            handle.unbind();    // Permanently remove
        });
    }
};
```

## See Also

- [Reactor](reactor.md)
- [PowerPlant](power-plant.md)
