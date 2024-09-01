# Get

The `Get` concept in NUClear's DSL (Domain Specific Language) is used to retrieve data required for a reaction.
It allows a reaction to specify dependencies on certain types of data, which will be provided when the reaction is executed.

Each of the items returned by a `get` call must be `truthy` for the reaction to execute, that is they must evaluate to `true` when cast to a boolean.
If any of the items are `falsy`, the reaction will not execute.

When these arguments are passed into the reaction function, not all of them need to be present.
If an argument is not requested by the reaction function, it not be passed in.
However, even if the argument is not requested, the get call will still occur and the result must be `truthy`.

## How does Get work

The `Get` concept works by defining a `get` function within a DSL word.
This function is called when the reaction is executed, and it retrieves the necessary data.
The retrieved data is then passed to the reaction function as arguments.

Typically implementers of the `Get` concept will use a data store to retrieve the data.
Or they may use a thread local variable that is set by another part of the system before the reaction is created.

The type which is requested by a reaction can either be the type returned by the `get` or the type which results from dereferencing the type returned by the `get`.
For example, if the `get` returns a `std::shared_ptr<T>`, the type requested by the reaction can be `T`.

## Example of a DSL word that implements Get

One example of a DSL word that implements the Get concept is the With word.
The With word retrieves the data of a specific type from NUClear's cache.

```cpp
namespace NUClear {
namespace dsl {
    namespace word {
        template <typename DataType>
        struct With {
        public:
            template <typename DSL, typename T = DataType>
            static std::shared_ptr<const T> get() {
                return store::ThreadStore<std::shared_ptr<T>>::value == nullptr
                           ? store::DataStore<DataType>::get()
                           : *store::ThreadStore<std::shared_ptr<T>>::value;
            }
        };
    }
}
}
```

## Optional Parameters

The `Optional` keyword in NUClear's DSL (Domain Specific Language) is used to mark certain fields as optional in a `get` call. This allows the reaction function to execute even if some of the optional data is missing.

When using the `Optional` keyword, the return type of the `get` call is always "truthy", meaning it evaluates to `true` when cast to a boolean. This ensures that the reaction can execute even when it is missing some data.

By marking certain fields as optional, the reaction function can choose to use only the relevant arguments and ignore the rest. This is particularly useful when dealing with complex data structures where only a subset of the data is needed.

The `Optional` keyword ensures that the reaction function can still execute and perform its intended functionality, even if some of the optional data is not present.

### Example of Optional Parameters

In this example, we will demonstrate how to use the `Optional` keyword in conjunction with the `With` word in NUClear's DSL.

Let's say we have a reaction that needs to retrieve two types of data: `DataA` and `DataB`. However, `DataB` is optional and the reaction should still execute even if it is not present.

```cpp
on<Trigger<DataA>, Optional<With<DataB>>>().then([this](const DataA& a, const std::shared_ptr<const DataB>& b) {
  // b will be null if DataB has not been emitted
});
```

## Transients in Get

The concept of transients in the context of `Get` refers to data that is only temporarily available.
When a type is marked as transient, the system caches the last copy of the data provided.
If no new data is available, the cached data is used instead.

This commonly occurs when the data is only available while the associated reaction is executing.
For example, if you were to use `on<UDP, Trigger<X>>` to receive a UDP packet, the data would be transient.
Then if the reaction were triggered by a `Trigger<X>` message, the previous data for the UDP packet would be provided.

### Flagging types as transient

To mark a type as transient, you need to define a trait that specifies that the type is transient.

For example, consider a type `TransientMessage` that represents a transient message.
The `TransientMessage` type is an example of a transient type.
It is marked as transient using the `is_transient` trait.

```cpp
namespace NUClear {
namespace dsl {
    namespace trait {
        template <>
        struct is_transient<TransientMessage> : std::true_type {};
    }
}
}
```

In the reaction, the `Get` for the TransientMessage retrieves the data, and if it is not available, it returns an empty message.
When the reaction is executed, the last known value of the TransientMessage is used if the data is not freshly available.
If no data has been received yet, the reaction will not execute.

```cpp
struct TransientGetter : NUClear::dsl::operation::TypeBind<TransientMessage> {
    template <typename DSL>
    static TransientMessage get(NUClear::threading::ReactionTask& task) {
        auto raw = NUClear::dsl::operation::CacheGet<TransientMessage>::get<DSL>(task);
        if (raw == nullptr) {
            return {};
        }
        return *raw;
    }
};
```

This ensures that the reaction can still execute even if the transient data is not freshly available, by using the last known value.
