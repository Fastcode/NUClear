# Optional

Wraps DSL words to allow a reaction to proceed when their data is unavailable, rather than dropping the task.

## Syntax

```cpp
on<Trigger<T>, Optional<With<U>>>()
```

## Parameters

| Parameter     | Description                                       |
| ------------- | ------------------------------------------------- |
| `DSLWords...` | One or more DSL words that provide data via `get` |

## Behavior

By default, if a word like `With<T>` references data that has not yet been emitted, the task is **dropped** — the callback never executes. `Optional` changes this: the task proceeds regardless, but the data may be absent.

Wrapping a word in `Optional` changes the corresponding callback parameter type:

| Expression          | Callback Parameter Type    | On Missing Data |
| ------------------- | -------------------------- | --------------- |
| `With<T>`           | `const T&`                 | Task dropped    |
| `Optional<With<T>>` | `std::shared_ptr<const T>` | `nullptr`       |

`Optional` can wrap any word whose fusion operation provides data via `get`, including `With`, `Trigger`, and `Last`.

!!! warning

    You **must** check for `nullptr` before dereferencing the shared pointer. Accessing a null pointer is undefined behavior.

## Example

```cpp
// Without Optional: task dropped if Config hasn't been emitted
on<Trigger<Data>, With<Config>>().then([](const Data& d, const Config& c) {
    // Only runs when both Data and Config exist
});

// With Optional: task runs regardless, check for nullptr
on<Trigger<Data>, Optional<With<Config>>>().then(
    [](const Data& d, std::shared_ptr<const Config> c) {
        if (c) {
            // Config is available
        }
        else {
            // Config hasn't been emitted yet — use defaults
        }
    });
```

## Notes

- Multiple words can be wrapped independently: `Optional<With<A>>, Optional<With<B>>`
- `Optional` only affects data availability checks — triggering semantics are unchanged
- A triggered reaction with all-optional data words will always execute when triggered

## See Also

- [With](with.md) — attach non-triggering data to a reaction
- [Trigger](trigger.md) — trigger a reaction when data is emitted
- [Last](last.md) — access the most recent N emissions of a type
