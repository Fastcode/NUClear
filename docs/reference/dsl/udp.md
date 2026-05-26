# UDP

Triggers a reaction when a raw UDP packet is received on a bound port.

## Syntax

```cpp
on<UDP>(port)
on<UDP::Broadcast>(port)
on<UDP::Multicast>(multicast_address, port)
```

## Modes

| Mode             | Arguments                  | Description                              |
| ---------------- | -------------------------- | ---------------------------------------- |
| `UDP`            | `port`                     | Listen for unicast packets on `port`     |
| `UDP::Broadcast` | `port`                     | Listen for broadcast packets on `port`   |
| `UDP::Multicast` | `multicast_address`, `port`| Join a multicast group and listen        |

## Parameters

| Parameter           | Type          | Description                                      |
| ------------------- | ------------- | ------------------------------------------------ |
| `port`              | `in_port_t`  | The UDP port to bind and listen on               |
| `multicast_address` | `std::string` | Multicast group address to join (multicast only) |

## Callback Argument

The callback receives a `const UDP::Packet&` with the following fields:

| Field    | Type                    | Description                                  |
| -------- | ----------------------- | -------------------------------------------- |
| `valid`  | `bool`                  | Whether the packet was received successfully |
| `local`  | address struct          | Local address the packet was received on     |
| `remote` | address struct          | Remote address of the sender                 |
| `payload`| `std::vector<uint8_t>`  | Raw packet data                              |

## Behavior

- Binds a UDP socket on the specified port when the reaction is created.
- The reaction fires each time a packet arrives on that socket.
- For `UDP::Multicast`, the system joins the specified multicast group via IGMP.
- No serialization or framing is applied — data is delivered as raw bytes.
- No peer discovery or connection management is performed.

## Example

```cpp
on<UDP>(5000).then([](const UDP::Packet& packet) {
    // Raw UDP data on port 5000
    auto& data = packet.payload;
    auto& sender = packet.remote;
});

on<UDP::Broadcast>(6000).then([](const UDP::Packet& packet) {
    // Broadcast traffic on port 6000
});

on<UDP::Multicast>("239.226.152.162", 7447).then([](const UDP::Packet& p) {
    // Multicast group traffic
});
```

## Notes

- This provides **raw UDP** access. There is no message typing, serialization, or automatic peer discovery.
- Use this when interoperating with non-NUClear systems or protocols that require direct UDP access.
- For typed, serialized messaging with automatic peer discovery between NUClear nodes, use [Network](network.md) instead.
- Packets can also be sent using the [UDP emit](../emit/udp.md).

## See Also

- [Network](network.md) — higher-level typed messaging with peer discovery
- [TCP](tcp.md) — stream-oriented connections
- [IO](io.md) — react on arbitrary file descriptors
- [UDP emit](../emit/udp.md) — sending UDP packets
- [How-to: TCP/UDP](../../how-to/tcp-udp.md) — practical guide
