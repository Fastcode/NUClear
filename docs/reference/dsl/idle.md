# Idle

Executes a task when a thread pool has no other work to process.

## Syntax

```cpp
on<Idle<PoolType>>().then(callback);  // Fires when PoolType's pool is idle
on<Idle<>>().then(callback);          // Fires when the default pool is idle
```

## Parameters

| Parameter  | Description                                              |
| ---------- | -------------------------------------------------------- |
| `PoolType` | The thread pool to monitor for idleness. Defaults to the default pool if omitted. |

## Behavior

- Registers the task as an idle handler for the specified pool
- The task executes on the targeted pool's threads when no higher-priority work is queued
- When new work arrives for the pool, running idle tasks yield execution
- The task may be invoked repeatedly whenever the pool becomes idle again

### Pool-Specific Idle vs Default Pool Idle

There are distinct idle states in NUClear:

| Variant | Meaning |
|---------|---------|
| `Idle<>` | The **default** thread pool has no queued tasks |
| `Idle<MyPool>` | A **specific** named pool has no queued tasks |

Each pool tracks its own idle state independently. A specific pool being idle does not mean the default pool is idle, and vice versa. If you have multiple pools, each can have its own `Idle` reactions that fire independently when that specific pool has no work.

## Example

```cpp
// Run when the default pool is idle
on<Idle<>>().then([] {
    do_background_cleanup();
});

// Run when a specific pool is idle
struct ComputePool {
    static constexpr const char* name = "Compute";
    static constexpr int concurrency = 4;
};

on<Idle<ComputePool>>().then([] {
    // Background work on ComputePool threads when it has nothing else to do
});
```

## Notes

- `Idle` is distinct from `Priority::IDLE`. `Idle` triggers only when the pool is completely free of work, whereas `Priority::IDLE` assigns the lowest scheduling priority to a task that is still queued normally.
- Suitable for housekeeping, cache warming, or other background work that should not compete with active processing.

## See Also

- [Always](always.md) — continuous execution regardless of pool state
- [Every](every.md) — periodic time-based execution
- [Pool](pool.md) — defining custom thread pools
- [Priority](priority.md) — task scheduling priority levels
