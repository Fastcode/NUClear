# precondition

Called before a task is queued. If any precondition returns `false`, the task is dropped.

## Signature

```cpp
template <typename DSL>
static bool precondition(threading::ReactionTask& task)
```

## Details

| Aspect      | Detail                                                                                        |
| ----------- | --------------------------------------------------------------------------------------------- |
| **When**    | After task creation, before `get` and submission                                              |
| **Thread**  | The emitter's thread (the thread that called `emit()`)                                        |
| **Returns** | `bool` — `true` to allow, `false` to drop                                                     |
| **Fusion**  | Logical AND with short-circuit evaluation. First `false` stops evaluation and drops the task. |

## Context & Arguments

- `task` — the `ReactionTask` being evaluated. Access to `task.reaction` gives you the parent Reaction's state (e.g., `active_tasks` count, enabled state).

Runs on the **emitter's thread**. The precondition check happens synchronously during the emit loop — before the task is submitted to any scheduler queue. This means precondition checks should be fast and non-blocking.

When a task is dropped by a precondition, its status is set to `BLOCKED`.

## Example

```cpp
struct Single {
    template <typename DSL>
    static bool precondition(threading::ReactionTask& task) {
        // Only allow one instance of this reaction at a time
        return task.reaction->active_tasks.load() == 0;
    }
};
```

## Built-in Words Using precondition

- `Single` — blocks if the reaction already has an active task
- `Buffer<N>` — blocks if N tasks are already active/queued
- Reaction enable/disable — disabled reactions always fail precondition
