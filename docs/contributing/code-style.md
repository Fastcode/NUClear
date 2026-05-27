# Code Style

Coding conventions for the NUClear codebase.

## Language Standard

- **C++14** — All code must compile under C++14
- Use standard library features over third-party alternatives where practical

## Naming

| Element                 | Convention                                 | Example                           |
| ----------------------- | ------------------------------------------ | --------------------------------- |
| Variables, functions    | `snake_case`                               | `log_level`, `emit_data()`        |
| Types, classes, structs | `PascalCase`                               | `PowerPlant`, `ReactionTask`      |
| Template parameters     | `PascalCase`                               | `typename T`, `typename DSLWords` |
| Constants               | `UPPER_SNAKE_CASE` or `constexpr` variable | `LogLevel::INFO`                  |
| Namespaces              | `snake_case`                               | `NUClear::dsl::word`              |

## Formatting

- **4-space indentation** (no tabs)
- Opening braces on the same line as the declaration
- Maximum line length: 120 characters
- Use `clang-format` (configuration is in the repository)

```cpp
class MyReactor : public NUClear::Reactor {
public:
    explicit MyReactor(std::unique_ptr<NUClear::Environment> environment)
        : NUClear::Reactor(std::move(environment)) {
        // ...
    }
};
```

## Headers

- Use `#pragma once` for include guards
- Order includes: related header, C++ standard library, third-party, project headers
- Use IWYU pragmas where appropriate (`// IWYU pragma: export`)

## Template Metaprogramming

- Use `PascalCase` for template type parameters
- Prefer `static constexpr` members over macros
- Use `std::enable_if_t` or `if constexpr` for SFINAE/conditional compilation
- Keep template instantiation depth reasonable

## DSL Words

- Each DSL word lives in its own header under `src/dsl/word/`
- DSL words are header-only
- Keep DSL word files self-contained — minimize dependencies between words
- Implement the required static interface methods (`bind`, `get`, `precondition`, `postcondition`, `reschedule`)

## Tests

- One test file per feature or DSL word
- Use Catch2 (`catch2/catch_test_macros.hpp`)
- Use `test_util::TestBase` for reactor-based tests
- Test file names should describe what is being tested (e.g., `Every.cpp`, `EmitFusion.cpp`)
- Use anonymous namespaces for test-internal state

## General Guidelines

- Prefer `std::unique_ptr` over raw pointers for ownership
- Use `const` and `constexpr` wherever applicable
- Avoid macros except where absolutely necessary
- Keep functions short and focused
- Document non-obvious code with comments explaining *why*, not *what*
