# Postcondition

A **postcondition** in NUClear is a condition that is executed after a reaction has run.
Postconditions are used to perform cleanup or additional actions once the main reaction logic has completed.

## How it Works

Postconditions are defined as static methods within the DSL (Domain Specific Language) words and are executed after the reaction's main callback has finished. They are useful for tasks such as unbinding reactions, logging, or other post-reaction activities.

Postconditions will only run if the reaction itself runs. If a `Precondition` fails or a `Get` returns a falsey value, the postconditions will not be executed.

## Usage

To use a postcondition, you need to include it in the DSL chain when defining a reaction. The postcondition will be executed each time the reaction's main callback completes.

### Example: Using the `Once` Postcondition

The `Once` postcondition ensures that a reaction only runs once by unbinding the reaction after it has executed.

#### Code Example

```cpp
#include <nuclear>

struct TestReactor : public NUClear::Reactor {
    struct SimpleMessage {
        SimpleMessage(int run) : run(run) {}
        int run = 0;
    };

    TestReactor(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {

        // This reaction will only run once
        on<Trigger<SimpleMessage>, Once>().then([this](const SimpleMessage& msg) {
            std::cout << "Once Trigger executed " << msg.run << std::endl;
        });

        // This reaction will run multiple times
        on<Trigger<SimpleMessage>>().then([this](const SimpleMessage& msg) {
            std::cout << "Normal Trigger Executed " << msg.run << std::endl;
            if (msg.run < 10) {
                emit(std::make_unique<SimpleMessage>(msg.run + 1));
            }
            else {
                powerplant.shutdown();
            }
        });

        on<Startup>().then([this] {
            emit(std::make_unique<SimpleMessage>(0));
        });
    }
};
```

In this example, the `Once` postcondition is used to ensure that the reaction to `SimpleMessage` only runs once. After the reaction runs, the `Once` postcondition unbinds the reaction, preventing it from running again.

## Summary

Postconditions in NUClear provide a way to perform additional actions after a reaction has executed. By using postconditions like `Once`, you can ensure that reactions only run under certain conditions, improving the control and flexibility of your application. Remember that postconditions will only run if the reaction itself runs, so if a `Precondition` fails or a `Get` returns a falsey value, the postconditions will not be executed.
