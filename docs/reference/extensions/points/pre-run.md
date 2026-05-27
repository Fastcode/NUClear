# pre_run

Called immediately before the user callback executes, after the task has been picked up by a thread and scope has been acquired.

## Signature

```cpp
template <typename DSL>
static void pre_run(threading::ReactionTask& task)
```

## Details

| Aspect      | Detail                                                        |
| ----------- | ------------------------------------------------------------- |
| **When**    | After scope acquisition, before callback                      |
| **Thread**  | The executor thread (the thread pool thread running the task) |
| **Returns** | `void`                                                        |
| **Fusion**  | All words with `pre_run` are called sequentially              |

## Context & Arguments

- `task` — the `ReactionTask` about to execute. Provides access to the reaction identity, task ID, and any metadata set during creation.

Runs on the **executor thread** — the same thread that will execute the callback. Scope locks are already held at this point. Use this for setup that needs to happen right before the callback on the same thread (e.g., setting thread-local state, recording start timestamps).

## Example

```cpp
struct MyTracing {
    template <typename DSL>
    static void pre_run(threading::ReactionTask& task) {
        // Record the start time for this task
        thread_local_start_time = std::chrono::steady_clock::now();
    }
};
```

## Built-in Words Using pre_run

No built-in DSL words currently use `pre_run`. It is available for custom extensions that need per-execution setup on the executor thread.
