# group

Specifies concurrency group(s) that the task belongs to. Groups limit how many tasks sharing a group can run simultaneously.

## Signature

```cpp
template <typename DSL>
static std::set<std::shared_ptr<const GroupDescriptor>> group(threading::ReactionTask& task)
```

## Details

| Aspect | Detail |
|--------|--------|
| **When** | Task creation (determines concurrency constraints) |
| **Thread** | The emitter's thread (the thread that called `emit()`) |
| **Returns** | Set of group descriptors, each with a name and concurrency limit |
| **Fusion** | Set union — all groups from all words are merged. The task must satisfy all group constraints simultaneously. |

## Context & Arguments

- `task` — the `ReactionTask` being constructed.

Runs on the **emitter's thread** during task creation. The scheduler checks all group constraints before dispatching: a task can only start if, for every group it belongs to, the number of currently-running tasks in that group is below the group's concurrency limit.

## Example

```cpp
template <typename Group>
struct Sync {
    template <typename DSL>
    static std::set<std::shared_ptr<const GroupDescriptor>> group(threading::ReactionTask& /*task*/) {
        static const auto descriptor = std::make_shared<GroupDescriptor>(
            util::demangle(typeid(Group).name()), 1  // concurrency = 1
        );
        return {descriptor};
    }
};
```

## Built-in Words Using group

- `Sync<G>` — creates a group with concurrency limit of 1 (mutual exclusion)
- `Group<G, N>` — creates a group with concurrency limit of N
