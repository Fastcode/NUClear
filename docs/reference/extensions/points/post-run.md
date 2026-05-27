# post_run

Called immediately after the user callback completes, before scope locks are released.

## Signature

```cpp
template <typename DSL>
static void post_run(threading::ReactionTask& task)
```

## Details

| Aspect      | Detail                                                        |
| ----------- | ------------------------------------------------------------- |
| **When**    | After callback, before scope release                          |
| **Thread**  | The executor thread (the thread pool thread running the task) |
| **Returns** | `void`                                                        |
| **Fusion**  | All words with `post_run` are called sequentially             |

## Context & Arguments

- `task` — the `ReactionTask` that just finished executing. Provides access to the reaction identity and task state.

Runs on the **executor thread** — the same thread that ran the callback. Scope locks are still held at this point. Use this for cleanup or follow-up actions that should happen immediately after the callback, before any locks are released (e.g., emitting transient data, recording end timestamps).

## Example

```cpp
struct MyTracing {
    template <typename DSL>
    static void post_run(threading::ReactionTask& task) {
        auto duration = std::chrono::steady_clock::now() - thread_local_start_time;
        record_timing(task.reaction->id, duration);
    }
};
```

## Built-in Words Using post_run

No built-in DSL words currently use `post_run`. It is available for custom extensions that need per-execution teardown on the executor thread.
