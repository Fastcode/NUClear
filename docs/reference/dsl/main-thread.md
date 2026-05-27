# MainThread

Forces task execution onto the main thread—the thread that called `PowerPlant::start()`.

## Syntax

```cpp
on<Trigger<T>, MainThread>().then([](const T& data) {
    // Runs on the main thread
});
```

## Behavior

`MainThread` is a [Pool](pool.md) with a concurrency of 1 that exclusively uses the main thread.
Only one `MainThread` task executes at a time.
If the main thread is occupied, subsequent tasks queue until it becomes available.

## Example

```cpp
on<Trigger<RenderCommand>, MainThread>().then([](const RenderCommand& cmd) {
    // Guaranteed to run on the main thread
    glDrawArrays(...);
});
```

## Notes

- The main thread is defined as the thread that called `PowerPlant::start()`.
- Useful for APIs that require main-thread access (OpenGL, macOS AppKit, some GUI frameworks).

!!! warning

    ```
    Scheduling too many `MainThread` tasks can create a bottleneck, since they execute sequentially on a single thread.
    ```

## See Also

- [Pool](pool.md)
- [Sync](sync.md)
