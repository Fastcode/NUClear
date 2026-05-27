# Writing Unit Tests for Reactors

> How to write deterministic unit tests for NUClear reactors using Catch2.

## Pattern

The standard pattern for testing a reactor:

1. Create a [`PowerPlant`](../reference/api/power-plant.md) with single-threaded execution
1. Install your test reactor
1. Start the plant (runs until idle or shutdown)
1. Assert on collected results

```cpp
NUClear::Configuration config;
config.default_pool_concurrency = 1;  // Deterministic ordering
NUClear::PowerPlant plant(config);
plant.install<TestReactor>();
plant.start();
// Assertions after plant shuts down
```

## Complete Example

```cpp
#include <catch2/catch_test_macros.hpp>
#include <vector>
#include <string>
#include "nuclear"

// The message types
struct InputData {
    int value;
};
struct OutputData {
    int result;
};

// The reactor under test
class Doubler : public NUClear::Reactor {
public:
    Doubler(std::unique_ptr<NUClear::Environment> environment)
        : Reactor(std::move(environment)) {

        on<Trigger<InputData>>().then([this](const InputData& input) {
            emit(std::make_unique<OutputData>(OutputData{input.value * 2}));
        });
    }
};

// Test reactor that drives the test and collects results
class TestReactor : public NUClear::Reactor {
public:
    TestReactor(std::unique_ptr<NUClear::Environment> environment)
        : Reactor(std::move(environment)) {

        on<Startup>().then([this] {
            emit(std::make_unique<InputData>(InputData{21}));
        });

        on<Trigger<OutputData>>().then([this](const OutputData& output) {
            results.push_back(output.result);
        });

        // Shut down when idle (nothing left to process)
        on<Idle<>>().then([this] {
            powerplant.shutdown();
        });
    }

    std::vector<int> results;
};

TEST_CASE("Doubler produces correct output", "[doubler]") {
    NUClear::Configuration config;
    config.default_pool_concurrency = 1;
    NUClear::PowerPlant plant(config);

    plant.install<Doubler>();
    const auto& test = plant.install<TestReactor>();
    plant.start();

    REQUIRE(test.results.size() == 1);
    REQUIRE(test.results[0] == 42);
}
```

## Key Techniques

### Single-Threaded Execution

Set `default_pool_concurrency = 1` to eliminate thread scheduling non-determinism. Tasks execute in a predictable order based on priority and submission time.

### Shutdown Strategies

| Strategy                                                               | When to use                                |
| ---------------------------------------------------------------------- | ------------------------------------------ |
| `on<`[`Idle`](../reference/dsl/idle.md)`<>>` → `powerplant.shutdown()` | Test completes when all reactions drain    |
| Explicit `powerplant.shutdown()` in a callback                         | You know exactly when the test is done     |
| `powerplant.shutdown(true)`                                            | Force immediate shutdown (timeout/failure) |

### Collecting Results

Store events in a container on your test reactor, then assert after `plant.start()` returns:

```cpp
class TestReactor : public NUClear::Reactor {
public:
    // ...
    std::vector<std::string> events;
};

TEST_CASE("...") {
    // ...
    const auto& reactor = plant.install<TestReactor>();
    plant.start();

    REQUIRE(reactor.events == expected);
}
```

### Stepping Through a Test

Use a step pattern to emit data in stages, ensuring each stage completes before the next:

```cpp
template <int i>
struct Step {};

on<Startup>().then([this] {
    emit(std::make_unique<Step<1>>());
});

on<Trigger<Step<1>>>().then([this] {
    // First stage
    emit(std::make_unique<SomeInput>(/*...*/));
    emit(std::make_unique<Step<2>>());
});

on<Trigger<Step<2>>>().then([this] {
    // Second stage - previous emissions have been processed
    emit(std::make_unique<AnotherInput>(/*...*/));
});
```

### Testing with Timeouts

Guard against tests that hang by using a timeout mechanism:

```cpp
on<Always>().then([this] {
    std::this_thread::sleep_for(std::chrono::seconds(5));
    if (!done) {
        FAIL("Test timed out");
        powerplant.shutdown(true);
    }
});
```

## Tips

- `plant.install<T>()` returns a `const T&` reference to the installed reactor — use it to read results after shutdown.
- Install all reactors before calling `plant.start()`.
- `plant.start()` blocks until `powerplant.shutdown()` is called.
- Keep test reactors focused — one test per behavior, one assertion concern per test reactor.
