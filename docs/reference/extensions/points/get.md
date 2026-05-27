# get

Called during task creation to retrieve data for the user's callback. The return value becomes one of the callback arguments.

## Signature

```cpp
template <typename DSL>
static T get(threading::ReactionTask& task)
```

## Details

| Aspect      | Detail                                                                                                                                         |
| ----------- | ---------------------------------------------------------------------------------------------------------------------------------------------- |
| **When**    | After preconditions pass, during task creation                                                                                                 |
| **Thread**  | The emitter's thread (the thread that called `emit()`)                                                                                         |
| **Returns** | Data of any type — becomes a callback parameter                                                                                                |
| **Fusion**  | Each word's return is concatenated into a flattened tuple. The tuple elements are matched to callback parameters via greedy argument matching. |

## Context & Arguments

- `task` — the `ReactionTask` being constructed. Provides access to the parent `Reaction`, the current `cause` (which reaction/task triggered this emit), and task metadata.

The `get` function runs on the **emitter's thread** — the thread that called `emit()`. This is important: if your `get` reads from `ThreadStore`, it will see the data that the emitter set. If it reads from `DataStore`, it gets the global latest value.

If `get` returns `nullptr` (for a pointer/shared_ptr type), the task is dropped with status `MISSING_DATA` — unless the word is wrapped in `Optional`, which converts the null to a valid empty state.

## Example

```cpp
struct With {
    template <typename DSL>
    static std::shared_ptr<const T> get(threading::ReactionTask& /*task*/) {
        // Return the latest value from the global data store
        return store::DataStore<T>::get();
    }
};
```

```cpp
struct Trigger {
    template <typename DSL>
    static std::shared_ptr<const T> get(threading::ReactionTask& /*task*/) {
        // Return the data from the current emit (thread-local)
        auto* ptr = store::ThreadStore<std::shared_ptr<T>>::value;
        if (ptr != nullptr) { return *ptr; }
        // Fall back to global store
        return store::DataStore<T>::get();
    }
};
```

## Built-in Words Using get

- `Trigger<T>` — reads from ThreadStore (triggering data), falls back to DataStore
- `With<T>` — reads from DataStore (latest value)
- `Last<N, T>` — returns a list of the last N triggered values
- `Network<T>` — deserialises the received bytes into `T`
- `IO` — returns the `IO::Event` struct (fd + event flags)
