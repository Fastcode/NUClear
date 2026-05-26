# Last

> Stores the last N emissions of a type and provides them as an ordered list.

## Syntax

```cpp
on<Last<N, Trigger<T>>>()
on<Trigger<T1>, Last<N, With<T2>>>()
```

## Parameters

| Parameter    | Description                                          |
| ------------ | ---------------------------------------------------- |
| `N`          | The maximum number of recent emissions to store.     |
| `DSLWords…`  | The DSL word to wrap (`Trigger<T>` or `With<T>`).    |

## Behavior

`Last` maintains a sliding window of the most recent N emissions of the wrapped type. When the reaction executes, the callback receives the full window as a `std::list<std::shared_ptr<const T>>`, ordered oldest first.

- If fewer than N items have been emitted, the list contains only what is available.
- When a new emission arrives and the window is full, the oldest item is discarded.
- When wrapping `Trigger`, the reaction still fires on every emission of `T`.
- When wrapping `With`, the list is retrieved as supplementary data without triggering.

```mermaid
graph LR
    subgraph "Window (N=4)"
        direction LR
        A["oldest"] --- B["…"] --- C["…"] --- D["newest"]
    end

    E["new emission"] -->|pushes out oldest| A
    E -->|appended| D
```

## Example

```cpp
on<Last<10, Trigger<SensorReading>>>().then(
    [](const std::list<std::shared_ptr<const SensorReading>>& readings) {
        // readings contains up to 10 most recent readings, oldest first
        double avg = 0;
        for (const auto& r : readings) {
            avg += r->value;
        }
        avg /= readings.size();
    });
```

The reaction fires on each `SensorReading` emission. The callback receives the last 10 readings (or fewer if not yet emitted 10 times), enabling computations like moving averages.

## Notes

- The list is ordered oldest-first — `front()` is the oldest item, `back()` is the most recent.
- Items are held as `std::shared_ptr<const T>`, extending lifetime until they leave the window.
- Useful for moving averages, buffering sensor data, and maintaining state history.
- If no items have been emitted, the list is empty and the task may be dropped. Use `Optional` to handle this case.

## See Also

- [Trigger](trigger.md) — triggers the reaction on emission of a type.
- [With](with.md) — provides supplementary data without triggering.
- [Optional](optional.md) — prevents task dropping when data is unavailable.
