# Priority

Sets the scheduling priority for a reaction's tasks in the thread pool.

## Syntax

```cpp
on<Trigger<T>, Priority::REALTIME>()
on<Trigger<T>, Priority::HIGH>()
on<Trigger<T>, Priority::NORMAL>()
on<Trigger<T>, Priority::LOW>()
on<Trigger<T>, Priority::IDLE>()
```

## Levels

| Level | Value | Use case |
|-------|-------|----------|
| REALTIME | 1000 | Safety-critical, hard deadlines |
| HIGH | 750 | Important, time-sensitive |
| NORMAL | 500 | Default for all reactions |
| LOW | 250 | Background processing |
| IDLE | 0 | Only when nothing else to do |

## Behaviour

Higher priority tasks are dequeued before lower priority tasks when a thread becomes available. Priority affects only queuing order — it does not preempt tasks that are already running. Scheduling is cooperative.

If no priority is specified, `NORMAL` (500) is used.

Priority implements the `priority` extension point. If multiple DSL words in the same binding set a priority, the maximum value wins.

## Example

```cpp
on<Trigger<CriticalData>, Priority::REALTIME>().then([](const CriticalData& d) {
    // Scheduled before NORMAL priority tasks
});

on<Trigger<Background>, Priority::LOW>().then([](const Background& b) {
    // Yields to higher priority work in the queue
});
```

## Notes

- Priority does **not** preempt running tasks. A low-priority task already executing will not be interrupted by a high-priority task entering the queue.
- Default priority is `NORMAL` (500) when unspecified.
- Priority applies per-reaction, not per-reactor.

## See Also

- [Pool](pool.md) — control which thread pool executes the task
- [Group](group.md) — limit concurrent execution of reactions
- [Idle](idle.md) — trigger when the thread pool has no work
