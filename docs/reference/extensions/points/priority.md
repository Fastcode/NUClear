# priority

Returns the priority level for this task in the scheduling queue.

## Signature

```cpp
template <typename DSL>
static PriorityLevel priority(const threading::ReactionTask& task)
```

## Details

| Aspect      | Detail                                                                       |
| ----------- | ---------------------------------------------------------------------------- |
| **When**    | Task creation (determines queue ordering)                                    |
| **Thread**  | The emitter's thread (the thread that called `emit()`)                       |
| **Returns** | `PriorityLevel` — higher levels are scheduled before lower ones              |
| **Fusion**  | Maximum value wins. If multiple words provide priority, the highest is used. |

## Context & Arguments

- `task` — the `ReactionTask` being constructed.
    Typically the priority is a compile-time constant and doesn't depend on task state.

Runs on the **emitter's thread** during task creation.
The returned value determines where the task sits in the scheduler's priority queue.
Higher priority tasks are dequeued and executed before lower priority ones.

Priority does **not** preempt running tasks — it only affects queue ordering.
When a task runs, the OS thread priority is set via `ThreadPriority` RAII for the duration of the callback.

## Example

```cpp
struct HighPriority {
    template <typename DSL>
    static PriorityLevel priority(const threading::ReactionTask& /*task*/) {
        return PriorityLevel::HIGH;
    }
};
```

## Built-in Words Using priority

- `Priority::REALTIME`
- `Priority::HIGHEST`
- `Priority::HIGH`
- `Priority::NORMAL` — default
- `Priority::LOW`
- `Priority::LOWEST`
- `Priority::IDLE`

The scheduler maps these DSL levels onto five internal buckets (`REALTIME`, `HIGH`, `NORMAL`, `LOW`, `IDLE`).
`LOWEST` shares the `LOW` bucket; `HIGHEST` shares the `HIGH` bucket.
