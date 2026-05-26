# run_inline

Controls whether the task can be executed inline (on the emitting thread) rather than being submitted to a pool.

## Signature

```cpp
template <typename DSL>
static util::Inline run_inline(threading::ReactionTask& task)
```

## Details

| Aspect | Detail |
|--------|--------|
| **When** | Task creation (determines execution mode) |
| **Thread** | The emitter's thread (the thread that called `emit()`) |
| **Returns** | `Inline::ALWAYS`, `Inline::NEVER`, or `Inline::NEUTRAL` |
| **Fusion** | Must agree. `ALWAYS` + `NEVER` from different words throws an exception. `NEUTRAL` defers to non-neutral words. If all are `NEUTRAL`, behaviour depends on the emit scope. |

## Context & Arguments

- `task` — the `ReactionTask` being constructed.

Runs on the **emitter's thread** during task creation. When `ALWAYS` is returned, the task executes immediately on the current thread rather than being submitted to the scheduler. This is useful for operations that need to happen synchronously with the emit.

When `NEVER` is returned, the task is always queued, even if the emit scope would normally prefer inline execution.

## Example

```cpp
struct Inline<util::Inline::ALWAYS> {
    template <typename DSL>
    static util::Inline run_inline(threading::ReactionTask& /*task*/) {
        return util::Inline::ALWAYS;
    }
};
```

## Built-in Words Using run_inline

- `Inline<ALWAYS>` — forces immediate execution on the emitter's thread
- `Inline<NEVER>` — forces queued execution, even for inline emits
