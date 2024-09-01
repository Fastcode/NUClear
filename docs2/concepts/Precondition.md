# Precondition

A **precondition** in NUClear is a condition that must be met for a reaction to execute. If the precondition evaluates to `true`, the reaction will proceed as normal.
If it evaluates to `false`, the reaction will be skipped.

## How it Works

Preconditions are used to control the execution of reactions based on specific criteria.
They are defined as static methods within the DSL (Domain Specific Language) words and are checked before the reaction is executed. If any precondition returns `false`, the reaction will not run.

## Usage

To use a precondition, you need to include it in the DSL chain when defining a reaction. The precondition will be evaluated each time the reaction is triggered.

### Example: Using the `Single` Precondition

The `Single` precondition ensures that only one instance of the associated reaction can execute at any given time.
If the reaction is triggered while an existing task for this reaction is either in the queue or still executing, the new task request will be ignored.

#### Code Example

```cpp
#include <nuclear>

struct TestReactor : public NUClear::Reactor {
    TestReactor(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {

        on<Trigger<SomeEvent>, Single>().then([this] {
            // This code will only run if no other instance of this reaction is currently running
            std::cout << "Single reaction executed" << std::endl;
        });
    }
};
```

In this example, the `Single` precondition is used to ensure that the reaction to `SomeEvent` only runs if no other instance of the same reaction is currently running.

### Defining Custom Preconditions

You can also define your own preconditions by creating a struct with a static `precondition` method. This method should return a `bool` indicating whether the reaction should run.

#### Custom Precondition Example

```cpp
struct CustomPrecondition {
    template <typename DSL>
    static bool precondition(NUClear::threading::ReactionTask& task) {
        // Custom logic to determine if the reaction should run
        return some_condition_is_met();
    }
};

// Usage in a reaction
on<Trigger<SomeEvent>, CustomPrecondition>().then([this] {
    // This code will only run if CustomPrecondition returns true
    std::cout << "Custom precondition met, reaction executed" << std::endl;
});
```

In this example, `CustomPrecondition` is a user-defined precondition that checks a custom condition before allowing the reaction to execute.

## Summary

Preconditions in NUClear provide a powerful way to control the execution of reactions based on specific criteria.
By using preconditions like `Single`, you can ensure that reactions only run under certain conditions, improving the robustness and predictability of your application.
