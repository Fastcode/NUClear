# TaskScope

Uses thread-local storage to track when a task is executing within a named scope.
Other code can query whether it is currently running inside a particular `TaskScope<Group>`.

## Syntax

```cpp
on<Trigger<T>, TaskScope<Group>>()
```

## Parameters

| Parameter | Description                                                        |
| --------- | ------------------------------------------------------------------ |
| `Group`   | A type tag (typically an empty struct) that identifies this scope. |

## Behavior

`TaskScope` implements the `scope` extension point.
When the reaction's callback begins execution, an RAII guard is constructed that marks the scope as active in thread-local storage.
When the callback returns (or throws), the guard is destroyed and the scope is deactivated.

- The scope is active only for the duration of the callback.
- Scope state is per-thread — it does not propagate across thread boundaries.
- Multiple `TaskScope` words with different groups can be active simultaneously.

## Example

```cpp
struct ProcessingScope {};

on<Trigger<Data>, TaskScope<ProcessingScope>>().then([](const Data& d) {
    // TaskScope<ProcessingScope> is active for the duration of this callback
});
```

## Notes

- Useful for conditional behavior based on execution context, such as preventing recursive triggering.
- The `Group` type follows the same pattern as [Sync](sync.md) groups — an empty struct used purely as a type tag.

## See Also

- [Sync](sync.md)
- [Group](group.md)
