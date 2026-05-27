# bind

Called once when the reaction is created (at the `.then()` call site). Use this to register the reaction with event sources.

## Signature

```cpp
template <typename DSL>
static void bind(const std::shared_ptr<threading::Reaction>& reaction, Args... args)
```

## Details

| Aspect      | Detail                                                                                                                                                      |
| ----------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **When**    | Reaction registration time (inside `.then()`)                                                                                                               |
| **Thread**  | Main thread during construction, or whichever thread calls `on<>().then()` at runtime                                                                       |
| **Returns** | `void` (return values are discarded)                                                                                                                        |
| **Fusion**  | All words with `bind` are called sequentially. Runtime arguments (from `on<>(args...)`) are consumed left-to-right by whichever word's `bind` accepts them. |

## Context & Arguments

- `reaction` — the newly created Reaction object. Store a reference to it in whatever event registry you need (e.g., `TypeCallbackStore`, a timer queue, an IO poll set).
- `args...` — runtime arguments passed to `on<>(...)`. For example, `on<IO>(fd, IO::READ)` passes the file descriptor and event mask. Arguments are consumed greedily left-to-right across all words that have `bind` with matching parameters.

The thread context is typically the **main thread** during reactor construction. However, if `on<>().then()` is called at runtime (e.g., inside another reaction's callback), it runs on whichever thread the calling reaction is executing on.

## Example

```cpp
struct Trigger {
    template <typename DSL>
    static void bind(const std::shared_ptr<threading::Reaction>& reaction) {
        // Register this reaction to be notified when T is emitted
        store::TypeCallbackStore<T>::add(reaction);
    }
};
```

## Built-in Words Using bind

- `Trigger<T>` — registers in `TypeCallbackStore<T>`
- `Every` — submits a `ChronoTask` to ChronoController
- `IO` — registers a file descriptor with IOController
- `Watchdog` — submits a deadline task to ChronoController
- `Network<T>` — registers type hash with NetworkController
