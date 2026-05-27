# pool

Specifies which thread pool the task should execute on.

## Signature

```cpp
template <typename DSL>
static std::shared_ptr<const ThreadPoolDescriptor> pool(threading::ReactionTask& task)
```

## Details

| Aspect      | Detail                                                                                                 |
| ----------- | ------------------------------------------------------------------------------------------------------ |
| **When**    | Task creation (determines where the task runs)                                                         |
| **Thread**  | The emitter's thread (the thread that called `emit()`)                                                 |
| **Returns** | Pool descriptor (name + concurrency)                                                                   |
| **Fusion**  | Exactly one pool allowed. If multiple words provide `pool`, a compile-time or runtime error is raised. |

## Context & Arguments

- `task` — the `ReactionTask` being constructed. Provides context but typically the pool choice is static.

Runs on the **emitter's thread** during task creation. The returned descriptor tells the scheduler which thread pool queue to place this task in. The task will be picked up and executed by one of the threads in that pool.

If no word provides a `pool`, the task runs on the default pool.

## Example

```cpp
struct MainThread {
    template <typename DSL>
    static std::shared_ptr<const ThreadPoolDescriptor> pool(threading::ReactionTask& /*task*/) {
        static const auto descriptor = std::make_shared<ThreadPoolDescriptor>(
            "MainThread", 1  // single-threaded
        );
        return descriptor;
    }
};
```

## Built-in Words Using pool

- `Pool<P>` — routes to the user-defined pool `P`
- `MainThread` — routes to the single-threaded main thread pool
- `Always` — uses a dedicated persistent pool for continuous execution
