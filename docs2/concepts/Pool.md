# Pool

## What is a Pool

A Pool in NUClear is a thread pool that manages the execution of tasks. It allows for concurrent execution of tasks across multiple threads, improving the efficiency and performance of your application.

## How does a Pool work

A Pool works by maintaining a set of threads that are used to execute tasks. When a task is submitted to the Pool, it is queued and executed by one of the available threads. If no threads are available, the task waits until a thread becomes free.

## How to make a Pool

To create a custom thread pool in NUClear, you use the `Pool<Type>` DSL.
The `Type` struct defines the attributes of the thread pool, such as the number of threads, whether the pool participates in idle, and whether it persists after shutdown.

### Pool Concepts

#### Pool Size

The size of the thread pool is determined by the `thread_count` attribute in the `Type` struct.
This specifies the number of threads that will be created for the pool.

Example:

```cpp
struct MyPool {
    static constexpr int thread_count = 4;
};
```

#### Persistent Pool

The persistent pool concept, referred to as "ignore shutdown" in the code, is controlled by the `continue_on_shutdown` attribute.
If set to `true`, the pool will continue to accept tasks even after the system is shutting down.

Example:

```cpp
struct PersistentPool {
    static constexpr bool continue_on_shutdown = true;
};
```

#### Idle Participation

The `counts_for_idle` attribute determines whether the threads in the pool count towards the system's idle state.
If set to `true`, the pool's threads will be considered when determining if the system is idle.

Example:

```cpp
struct IdlePool {
    static constexpr bool counts_for_idle = true;
};
```

### Default Thread Pool

The default thread pool is created with a thread count specified at runtime when the `PowerPlant` is instantiated.
This allows for flexibility in configuring the number of threads based on the application's needs.

Example:

```cpp
NUClear::PowerPlant powerplant(config, argc, argv);
```

### MainThread Pool

The `MainThread` pool uses the thread that started the `PowerPlant`, which is typically the main thread. This is particularly useful in scenarios involving computer graphics, such as OpenGL, where certain functions must be executed on the main thread.

Example:

```cpp
on<Trigger<MyEvent>, MainThread>() {
    // This code will run on the main thread
}
```

### Example Usage

Here is an example of how to create and use a custom thread pool in NUClear:

```cpp
struct CustomPool {
    static constexpr int thread_count = 2;
    static constexpr bool counts_for_idle = false;
    static constexpr bool continue_on_shutdown = true;
};

on<Trigger<MyEvent>, Pool<CustomPool>>() {
    // This code will run in the custom thread pool
}
```

In this example, a custom thread pool named `CustomPool` is created with 2 threads, does not count towards idle, and continues to accept tasks after shutdown.
The `on<Trigger<MyEvent>, Pool<CustomPool>>()` syntax specifies that the reaction should run in this custom thread pool.

By understanding and utilizing these concepts, you can effectively manage the concurrency and performance of your NUClear application.
