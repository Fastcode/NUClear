### Understanding Time in the NUClear System

The NUClear system provides a robust and flexible way to manage time within your application. This document will guide you through the various time-related features available in NUClear, ensuring you understand how to use them effectively.

#### NUClear::clock Type

The `NUClear::clock` type is a specialized clock used within the NUClear system. It is based on the `std::chrono::system_clock` by default, but it can be overridden to use other clock types if needed. The `NUClear::clock` provides several functionalities:

- **Current Time**: You can get the current time using `NUClear::clock::now()`.
- **Adjusting the Clock**: The clock can be adjusted using `NUClear::clock::adjust_clock`.
- **Setting the Clock**: You can set the clock to a specific time using `NUClear::clock::set_clock`.
- **Real-Time Factor (RTF)**: The clock supports a real-time factor, which allows you to control the speed at which time progresses.

#### TimeTravel in the NUClear System

TimeTravel is a powerful feature in NUClear that allows you to manipulate the system clock and the rate at which time passes. This is particularly useful for testing and simulation purposes. The `NUClear::message::TimeTravel` struct provides the following actions:

- **RELATIVE**: Adjusts the clock and moves all chrono tasks with it.
- **ABSOLUTE**: Adjusts the clock to a target time and leaves chrono tasks where they are.
- **NEAREST**: Adjusts the clock to as close to the target as possible without skipping any chrono tasks.

Example usage:

```cpp
emit<Scope::INLINE>(std::make_unique<NUClear::message::TimeTravel>(
    NUClear::clock::time_point(adjustment), rtf, action));
```

#### ChronoTasks

ChronoTasks are tasks scheduled to run at specific times or intervals. They are managed by the `ChronoController` and can be influenced by TimeTravel. There are two primary ways to use ChronoTasks:

1. **Emit Delay**: Schedule a task to run after a specific delay.
2. **On Every**: Schedule a task to run at regular intervals.

Example of scheduling a ChronoTask:

```cpp
emit<Scope::INLINE>(std::make_unique<NUClear::dsl::operation::ChronoTask>(
    [](NUClear::clock::time_point&) {
        // Task code here
        return false; // Return true to reschedule
    },
    NUClear::clock::now() + std::chrono::seconds(10), // Schedule 10 seconds from now
    1));
```

TimeTravel influences ChronoTasks by adjusting their scheduled times based on the selected action (RELATIVE, ABSOLUTE, NEAREST).

#### Usage Clock

The usage clock records CPU thread time, providing insights into how much CPU time is being consumed by different parts of your application. This can be useful for performance monitoring and optimization.

### Conclusion

Understanding how time works in the NUClear system allows you to leverage its powerful features for scheduling tasks, simulating different time scenarios, and monitoring performance. By using `NUClear::clock`, TimeTravel, and ChronoTasks, you can create flexible and efficient applications that handle time-based operations with ease.
