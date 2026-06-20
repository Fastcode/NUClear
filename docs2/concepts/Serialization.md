### Serialization in the NUClear System

Serialization in the NUClear system is designed to convert data into a format that can be easily stored or transmitted and then reconstructed later. This is particularly useful for sending data over a network or saving it to a file. NUClear provides built-in support for serializing various types of data, including Plain Old Data (POD) types and iterables of POD types.

#### How Serialization Types Work

NUClear uses a template-based approach to serialization. The primary template is `Serialise<T, Check>`, where `T` is the type to be serialized, and `Check` is a type trait used to specialize the serialization process for different types. The `Serialise` template provides three static methods that must be implemented for each type:

- `static std::vector<uint8_t> serialise(const T& in)`: Converts the object `in` into a vector of bytes.
- `static T deserialise(const std::vector<uint8_t>& in)`: Reconstructs an object of type `T` from a vector of bytes.
- `static uint64_t hash()`: Returns a hash value that uniquely identifies the type `T`.

#### Built-in Serialization for POD Types

NUClear provides built-in serialization for POD types, which are simple data types like integers, floats, and plain structs. The serialization process for these types is straightforward:

1. **Serialization**: The data is converted into a sequence of bytes, taking into account the endianness and alignment requirements of the target system.
2. **Deserialization**: The byte sequence is converted back into the original data type.

For example, to serialize an integer, the system converts the integer into its binary representation and stores it in a byte vector. When deserializing, the binary data is read from the byte vector and converted back into an integer.

#### Serialization for Iterables of POD Types

NUClear also supports serialization for iterables of POD types, such as arrays, vectors, and lists. The process involves serializing each element of the iterable individually and then combining the results into a single byte vector. During deserialization, the byte vector is split back into individual elements, which are then deserialized into their original types.

#### Using Serialization in NUClear

To use serialization in NUClear, you typically don't need to interact with the `Serialise` template directly. Instead, you use higher-level constructs provided by the NUClear framework, such as the `Network` or `UDP` DSL words, which internally handle the serialization and deserialization of data.

For example, to send a message over the network, you might use the `Network` DSL word:

```cpp
on<Trigger<Network<MyMessage>>>().then([this](const MyMessage& msg) {
    // Process the received message
});
```

In this example, `MyMessage` is a user-defined type that has been specialized for serialization. The `Network` DSL word automatically serializes the message before sending it and deserializes it upon receipt.

#### Custom Serialization

If you need to serialize a custom type that is not a POD type or an iterable of POD types, you can specialize the `Serialise` template for your type. This involves implementing the three static methods (`serialise`, `deserialise`, and `hash`) to define how your type should be converted to and from a byte vector.

### Conclusion

Serialization in the NUClear system is a powerful feature that allows you to easily convert data into a format suitable for storage or transmission. With built-in support for POD types and iterables of POD types, as well as the ability to define custom serialization for more complex types, NUClear provides a flexible and efficient serialization mechanism that integrates seamlessly with its other features.
