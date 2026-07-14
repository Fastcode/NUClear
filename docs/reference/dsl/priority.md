# Priority

Sets the scheduling priority for a reaction's tasks in the thread pool.

## Syntax

```cpp
on<Trigger<T>, Priority::REALTIME>()
on<Trigger<T>, Priority::HIGHEST>()
on<Trigger<T>, Priority::HIGH>()
on<Trigger<T>, Priority::NORMAL>()
on<Trigger<T>, Priority::LOW>()
on<Trigger<T>, Priority::LOWEST>()
on<Trigger<T>, Priority::IDLE>()
```

## Levels

| Level    | Use case                        |
| -------- | ------------------------------- |
| REALTIME | Safety-critical, hard deadlines |
| HIGHEST  | Highest non-realtime priority   |
| HIGH     | Important, time-sensitive       |
| NORMAL   | Default for all reactions       |
| LOW      | Background processing           |
| LOWEST   | Lowest non-idle priority        |
| IDLE     | Only when nothing else to do    |

## Behavior

Higher priority tasks are dequeued before lower priority tasks when a thread becomes available.
Priority affects only queuing order — it does not preempt tasks that are already running.
Scheduling is cooperative.

If no priority is specified, `NORMAL` is used.

The scheduler maps the seven DSL levels onto five internal buckets.
`LOWEST` and `HIGHEST` share adjacent buckets with `LOW` and `HIGH` respectively.

Priority implements the `priority` extension point.
If multiple DSL words in the same binding set a priority, the maximum value wins.

When a task executes, the worker thread's OS priority is set via `ThreadPriority` RAII for the duration of the callback.

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

- Priority does **not** preempt running tasks.
    A low-priority task already executing will not be interrupted by a high-priority task entering the queue.
- Default priority is `NORMAL` when unspecified.
- Priority applies per-reaction, not per-reactor.

## See Also

- [Pool](pool.md) — control which thread pool executes the task
- [Group](group.md) — limit concurrent execution of reactions
- [Idle](idle.md) — trigger when the thread pool has no work
