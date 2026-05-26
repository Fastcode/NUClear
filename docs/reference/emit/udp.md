# Scope::UDP

> Sends serialized data as a raw UDP packet to a specified address and port.

## Syntax

```cpp
// Send to address and port
emit<Scope::UDP>(std::make_unique<T>(args...), "192.168.1.100", 9000);

// Send specifying source address and port
emit<Scope::UDP>(std::make_unique<T>(args...), "192.168.1.100", 9000, "0.0.0.0", 8000);
```

## Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `data` | `std::unique_ptr<T>` | The data to serialize and send |
| `to_addr` | `std::string` | Destination address (unicast, broadcast, or multicast) |
| `to_port` | `in_port_t` | Destination port (host endian) |
| `from_addr` | `std::string` | (Optional) Source address, or `""` for automatic (default: `""`) |
| `from_port` | `in_port_t` | (Optional) Source port, or `0` for automatic (default: `0`) |

## Behavior

When data is emitted with `Scope::UDP`:

1. The data is serialized using `util::serialise::Serialise<T>`.
2. A UDP socket is opened and the serialized payload is sent as a single datagram.
3. No NUClear-specific framing or headers are added — the raw serialized bytes are the packet payload.

There is no fragmentation, reliability, ordering, or peer discovery. The packet is fire-and-forget.

## Example

```cpp
#include <nuclear>

struct StatusMessage {
    uint32_t id;
    float battery;
};

class Reporter : public NUClear::Reactor {
public:
    explicit Reporter(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {

        on<Every<5, std::chrono::seconds>>().then([this] {
            emit<Scope::UDP>(std::make_unique<StatusMessage>(StatusMessage{1, 0.85f}),
                             "192.168.1.255", 5000);
        });
    }
};
```

## Notes

- The type must be serializable: either trivially copyable, or provide a `util::serialise::Serialise<T>` specialization.
- Supports unicast, broadcast, and multicast destination addresses.
- No NUClear protocol wrapping — suitable for interoperating with non-NUClear systems that expect raw data.
- Maximum payload size is limited by the network MTU (typically ~1472 bytes for Ethernet). No fragmentation is performed.
- The socket is opened and closed per emit. For high-frequency sending, consider the `UDP` DSL word with `IO` for lower overhead.

## See Also

- [Network](network.md) — for communicating with other NUClear peers using the NUClear protocol
- [UDP DSL word](../dsl/udp.md) — receiving UDP packets
