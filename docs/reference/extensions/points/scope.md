# scope

Returns an RAII lock object that is held for the duration of the callback execution (including `pre_run` and `post_run`).

## Signature

```cpp
template <typename DSL>
static Lock scope(threading::ReactionTask& task)
```

## Details

| Aspect      | Detail                                                                                   |
| ----------- | ---------------------------------------------------------------------------------------- |
| **When**    | Acquired before `pre_run`, released after `post_run`                                     |
| **Thread**  | The executor thread (the thread pool thread running the task)                            |
| **Returns** | Any RAII type — held in a tuple until task completion                                    |
| **Fusion**  | All scope objects from all words are acquired and held simultaneously via FunctionFusion |

## Context & Arguments

- `task` — the `ReactionTask` being executed.
    Provides access to the reaction identity and any task metadata.

Runs on the **executor thread** — the thread that actually runs the callback.
This is different from `bind`/`get`/`precondition` which run on the emitter's thread.

The returned object is stored in a tuple and destroyed (released) after `post_run` completes.
This makes it ideal for RAII patterns like mutex locks, file handles, or any resource that should be held for the callback's duration.

If your `scope` implementation needs to persist state between reactions (not just for one execution), use the `Scope` extension point's associated storage — objects stored there persist for the reaction's lifetime via RAII, enabling cleanup when the reaction is destroyed.

## Example

```cpp
template <typename Group>
struct Sync {
    template <typename DSL>
    static std::unique_lock<std::mutex> scope(threading::ReactionTask& /*task*/) {
        static std::mutex mutex;
        return std::unique_lock<std::mutex>(mutex);
    }
};
```

## Built-in Words Using scope

- `Sync<G>` — acquires a mutex for the synchronisation group
- `TaskScope<G>` — tracks task execution context for hierarchical task management
