# Bind

## Overview

In NUClear, the "bind" concept is a fundamental mechanism that allows functions to be executed when specific conditions are met. This is achieved by associating (or binding) functions to specific types or events. When the specified type or event occurs, the bound function is triggered and executed. This concept is essential for creating reactive systems where actions are performed in response to changes or events in the system.

## How Bind Works

The bind concept is implemented through various DSL (Domain Specific Language) words that define the conditions under which functions should be executed. These DSL words encapsulate the logic for binding functions to specific types or events, making it easier to create complex reactive behaviors.

Example: Trigger DSL Word
One example of a DSL word that implements the bind concept is the Trigger word. The Trigger word allows reactions to be triggered based on the emission of a specific type of data.

Usage
Here are some examples of how the Trigger DSL word can be used to bind functions to events:

Binding to a Specific Data Type

```cpp
on<Trigger<DataType>>().then([](const DataType& data) {
    // Handle the received data
});
```

In this example, the function is bound to the emission of DataType. When data of this type is emitted, the function is triggered and executed.

Binding with Runtime Arguments
In this example, the function is bound to the emission of DataType and can also take runtime arguments. This allows for more dynamic and flexible reactions based on runtime conditions.

```cpp
on<Trigger<DataType>>(runtime_argument).then([](const DataType& data) {
    // Handle the received data with the runtime argument
});
```

## Implementation

The Trigger DSL word is implemented in the src/dsl/word/Trigger.hpp file. It provides methods for binding functions to specific data types, including:

bind: Binds the function to the specified data type.
These methods encapsulate the logic for handling data emissions and provide a simple interface for binding functions to these events.
