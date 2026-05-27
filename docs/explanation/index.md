# Explanation

This section dives into the **concepts and design decisions** behind NUClear. Rather than telling you how to use a specific feature, these pages explain *why* things work the way they do.

If you've already followed the tutorials and know how to use NUClear, this is where you come to deepen your understanding — to see the bigger picture and appreciate the trade-offs that shaped the framework.

## Pages

| Page                              | What It Covers                                                                                |
| --------------------------------- | --------------------------------------------------------------------------------------------- |
| [Architecture](architecture.md)   | Why NUClear exists, the problems it solves, and the event-driven reactive pattern at its core |
| [Threading Model](threading.md)   | How tasks are scheduled across thread pools, priority queues, and group constraints           |
| [Lifecycle](lifecycle.md)         | The three phases of a NUClear system: initialisation, execution, and shutdown                 |
| [The DSL System](dsl-system.md)   | How `on<>().then()` works from top to bottom — template metaprogramming in action             |
| [Message Flow](message-flow.md)   | What happens when you emit data, from call site to reaction execution                         |
| [NUClearNet](nuclearnet.md)      | The peer-to-peer networking mesh that connects NUClear instances                              |
| [Serialization](serialization.md) | How data is converted for network transmission                                                |
