# Configuration

> Struct passed to the PowerPlant constructor to configure runtime parameters.

## Overview

`Configuration` controls the PowerPlant's thread pool sizing. Passed by value to the `PowerPlant` constructor.

## API

```cpp
namespace NUClear {

struct Configuration {
    /// Number of threads in the default thread pool.
    /// Defaults to std::thread::hardware_concurrency(), or 2 if that returns 0.
    int default_pool_concurrency;
};

}  // namespace NUClear
```

| Field                      | Type  | Default                         | Description                           |
| -------------------------- | ----- | ------------------------------- | ------------------------------------- |
| `default_pool_concurrency` | `int` | `hardware_concurrency()` or `2` | Number of threads in the default pool |

## Example

```cpp
#include <nuclear>

int main(int argc, const char* argv[]) {
    NUClear::Configuration config;
    config.default_pool_concurrency = 8;  // Use 8 threads

    NUClear::PowerPlant plant(config, argc, argv);
    plant.install<MyReactor>();
    plant.start();
}
```

## See Also

- [PowerPlant](power-plant.md)
