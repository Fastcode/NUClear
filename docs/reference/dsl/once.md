# Once

Ensures a reaction runs exactly once, then permanently unbinds itself.

## Syntax

```cpp
on<Trigger<T>, Once>().then([](const T& data) {
    // ...
});
```

`Once` can be combined with any triggering word.

## Behavior

When a reaction with `Once` executes for the first time, it automatically unbinds the reaction handle immediately after completion.
The reaction is fully unbound — not merely disabled.
It will never trigger again, even if the triggering type is re-emitted.

## Example

```cpp
on<Trigger<InitData>, Once>().then([](const InitData& data) {
    // Runs exactly once, then this reaction is removed
    initialize_system(data);
});
```

## Notes

- The reaction is **unbound**, not disabled. It cannot be re-enabled via its handle.
- Useful for one-time initialization, lazy setup, or responding to a single event.
- Equivalent to manually calling `unbind()` on the reaction handle at the end of the callback, but declarative.

## See Also

- [Single](single.md) — restrict a reaction to one concurrent execution
- [Startup](startup.md) — trigger once when the system starts
- [Managing Reactions](../../how-to/manage-reactions.md) — manually unbind or enable/disable reactions via `ReactionHandle`
