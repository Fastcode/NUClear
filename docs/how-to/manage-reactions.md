# Managing Reactions

> Enable, disable, and remove reactions at runtime.

## Problem

You need to toggle reactions on and off based on runtime conditions — for example, disabling a feature when a configuration flag changes, or removing a reaction entirely when it's no longer needed.

## Solution

Store the [`ReactionHandle`](../reference/api/reaction-handle.md) returned by `on<>().then()` and use its methods to control the reaction.

### ReactionHandle API

| Method      | Description                                                 |
| ----------- | ----------------------------------------------------------- |
| `enable()`  | Allow the reaction to be triggered (default state)          |
| `disable()` | Prevent the reaction from triggering; configuration is kept |
| `enabled()` | Returns `true` if the reaction is currently enabled         |
| `unbind()`  | Permanently remove the reaction from the system             |

### 1. Store the Handle

```cpp
ReactionHandle my_handle = on<Trigger<SensorData>>().then([](const SensorData& data) {
    process(data);
});
```

### 2. Toggle at Runtime

```cpp
// Disable when you need to pause
my_handle.disable();

// Re-enable when ready
my_handle.enable();

// Or set directly from a boolean
my_handle.enable(should_run);
```

### 3. Complete Example — Feature Toggle

```cpp
#include <nuclear>

struct SensorData {
    double value;
};

struct FeatureConfig {
    bool processing_enabled;
};

class FeatureToggle : public NUClear::Reactor {
public:
    explicit FeatureToggle(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {

        // Create the processing reaction and store its handle
        processing_handle = on<Trigger<SensorData>>().then([](const SensorData& data) {
            log<INFO>("Processing sensor value:", data.value);
        });

        // React to config changes and toggle the processing reaction
        on<Trigger<FeatureConfig>>().then([this](const FeatureConfig& cfg) {
            processing_handle.enable(cfg.processing_enabled);
            log<INFO>("Processing", cfg.processing_enabled ? "enabled" : "disabled");
        });
    }

private:
    ReactionHandle processing_handle;
};
```

### 4. Permanent Removal with unbind()

```cpp
// One-shot setup that removes itself after first execution
ReactionHandle setup_handle;
setup_handle = on<Trigger<InitData>>().then([this] (const InitData& data) {
    perform_one_time_setup(data);
    setup_handle.unbind();  // Never triggers again
});
```

!!! warning "unbind() is irreversible"

    ```
    Once `unbind()` is called, the reaction is permanently removed.
    ```

    It cannot be re-enabled.
    Use `disable()` if you might need the reaction again later.

!!! warning "Don't disable Always reactions"

    ```
    A reaction bound with `on<`[`Always`](../reference/dsl/always.md)`>` should not be disabled — it will continuously spin checking for new tasks even when disabled.
    ```

    Use a different triggering word if you need to toggle the behavior.
