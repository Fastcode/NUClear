# priority

Returns the priority level for this task in the scheduling queue.

## Signature

```cpp
template <typename DSL>
static int priority(const threading::ReactionTask& task)
```

## Details

| Aspect      | Detail                                                                       |
| ----------- | ---------------------------------------------------------------------------- |
| **When**    | Task creation (determines queue ordering)                                    |
| **Thread**  | The emitter's thread (the thread that called `emit()`)                       |
| **Returns** | `int` — higher values mean higher priority                                   |
| **Fusion**  | Maximum value wins. If multiple words provide priority, the highest is used. |

## Context & Arguments

- `task` — the `ReactionTask` being constructed. Typically the priority is a compile-time constant and doesn't depend on task state.

Runs on the **emitter's thread** during task creation. The returned value determines where the task sits in the scheduler's priority queue. Higher priority tasks are dequeued and executed before lower priority ones.

Priority does **not** preempt running tasks — it only affects queue ordering.

## Example

```cpp
struct HighPriority {
    template <typename DSL>
    static int priority(const threading::ReactionTask& /*task*/) {
        return 750;  // Same as Priority::HIGH
    }
};
```

## Built-in Words Using priority

- `Priority::REALTIME` — value 1000
- `Priority::HIGH` — value 750
- `Priority::NORMAL` — value 500 (default)
- `Priority::LOW` — value 250
- `Priority::IDLE` — value 0
