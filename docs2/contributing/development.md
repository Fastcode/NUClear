# Development Setup

How to set up a development environment for working on NUClear itself.

## Clone and Build

```bash
git clone https://github.com/Fastcode/NUClear.git
cd NUClear
cmake -B build -DBUILD_TESTS=ON
cmake --build build
```

## Run Tests

```bash
cd build
ctest
```

Tests use [Catch2](https://github.com/catchorg/Catch2) and are automatically downloaded during the CMake configure step.

## Build Documentation

```bash
pip install -r docs2/requirements.txt
mkdocs serve
```

The docs will be available at `http://127.0.0.1:8000`.

## Project Structure

| Directory | Contents |
|-----------|----------|
| `src/` | Core library — PowerPlant, Reactor, DSL words, threading, utilities |
| `src/dsl/word/` | Individual DSL word implementations (one header per word) |
| `src/extension/` | Built-in extensions (network, IO, etc.) |
| `src/threading/` | Scheduler, reactions, and thread pool management |
| `tests/` | Test suite organized by feature area |
| `tests/test_util/` | Shared test utilities (TestBase, TimeUnit, etc.) |
| `docs2/` | MkDocs documentation source |
| `cmake/` | CMake modules and configuration templates |

## Adding a New DSL Word

1. Create a header in `src/dsl/word/YourWord.hpp`
2. Implement the required static methods (`bind`, `get`, `precondition`, etc.)
3. Add it to the forward declarations in `src/Reactor.hpp`
4. Add a `using` alias in the Reactor class

See [Extending the DSL](../how-to/extending-dsl.md) for the full guide on DSL word interfaces.

## Adding a Test

1. Create a `.cpp` file in the appropriate `tests/tests/` subdirectory
2. Include `<catch2/catch_test_macros.hpp>` and `"nuclear"`
3. Use `test_util::TestBase` as a base class for reactor-based tests
4. Register the test in the corresponding `CMakeLists.txt`

Example structure:

```cpp
#include <catch2/catch_test_macros.hpp>
#include <nuclear>
#include "test_util/TestBase.hpp"

// Define test messages and reactors...

TEST_CASE("Description of what is being tested", "[category]") {
    NUClear::Configuration config;
    config.default_pool_concurrency = 1;
    NUClear::PowerPlant plant(config);
    plant.install<TestReactor>();
    plant.start();

    // Verify results...
}
```
