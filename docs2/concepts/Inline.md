# Inline

The `Inline` concept in NUClear allows you to control whether a reaction should be executed inline or in its target thread pool. This can be useful for optimizing performance and ensuring that certain tasks are executed in a specific manner.

## What is an inline task

An inline task is a reaction that is executed in the same thread as the emitter, rather than being scheduled in a separate thread pool. This can reduce the overhead of context switching and improve performance for lightweight tasks.

## How are they created

Inline tasks are created using the `emit<Scope::INLINE>` function or by specifying the `Inline::ALWAYS` keyword in the reaction definition.

## When will a task run inline

A task will run inline based on the following conditions:

### Inline emit

Using `emit<Scope::INLINE>` will attempt to run the emitted task inline. However, this can be overridden by the reaction's inline level.

### Inline::NEVER

The `Inline::NEVER` keyword can be used in the reaction definition to ensure that the reaction is never executed inline, even if it is emitted with `Scope::INLINE`.

```cpp
on<Trigger<SimpleMessage>, Inline::NEVER>().then([](const SimpleMessage& message) {
    // This reaction will never run inline
});
```

### Inline::ALWAYS

The `Inline::ALWAYS` keyword can be used in the reaction definition to ensure that the reaction is always executed inline, even if it is not emitted with `Scope::INLINE`.

```cpp
on<Trigger<SimpleMessage>, Inline::ALWAYS>().then([](const SimpleMessage& message) {
    // This reaction will always run inline
});
```

### Groups

Inlining will never happen if the group lock prevents it from being inlined. This ensures that tasks that require synchronization are not executed inline, which could lead to race conditions or other concurrency issues.

## Example

Here is an example of how to use the `Inline` concept in a NUClear reactor:

```cpp
class TestReactor : public NUClear::Reactor {
public:
    TestReactor(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {

        on<Trigger<SimpleMessage>, Inline::ALWAYS>().then([](const SimpleMessage& message) {
            // This reaction will always run inline
        });

        on<Trigger<SimpleMessage>, Inline::NEVER>().then([](const SimpleMessage& message) {
            // This reaction will never run inline
        });

        on<Trigger<SimpleMessage>>().then([](const SimpleMessage& message) {
            // This reaction will run inline based on the emit scope
        });

        emit<Scope::INLINE>(std::make_unique<SimpleMessage>("Inline Message"));
    }
};
```

In this example, the first reaction will always run inline, the second reaction will never run inline, and the third reaction will run inline based on the emit scope.

## Summary

The `Inline` concept provides fine-grained control over how reactions are executed in NUClear. By using `Inline::NEVER` and `Inline::ALWAYS`, you can ensure that reactions are executed in the desired manner. Additionally, the `emit<Scope::INLINE>` function allows you to emit tasks inline, but this can be overridden by the reaction's inline level. Finally, inlining will never happen if the group lock prevents it from being inlined, ensuring that tasks requiring synchronization are not executed inline.

This documentation provides a comprehensive overview of the `Inline` concept, including how it works, how it can be controlled, and examples of its usage.
