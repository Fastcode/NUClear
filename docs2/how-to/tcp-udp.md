# Communicating with External Systems via TCP/UDP

> How to use raw TCP and UDP sockets to talk to non-NUClear systems.

## When to Use This vs Network\<T\>

| Feature | `Network<T>` | `TCP` / `UDP` |
|---------|-------------|---------------|
| Peer type | Other NUClear nodes | Any external system |
| Serialization | Automatic | Manual (raw bytes) |
| Discovery | Built-in multicast | You manage connections |
| Use case | Inter-node messaging | Protocols, hardware, third-party services |

## TCP

### Accepting Connections

Use `on<TCP>(port)` to listen for incoming TCP connections. When a client connects, you receive a `TCP::Connection` object with the file descriptor for the stream:

```cpp
#include "nuclear"

class TCPServer : public NUClear::Reactor {
public:
    TCPServer(std::unique_ptr<NUClear::Environment> environment)
        : Reactor(std::move(environment)) {

        // Listen on port 9000
        on<TCP>(9000).then([this](const TCP::Connection& connection) {
            log("Client connected from port", ntohs(connection.remote.ipv4.sin_port));

            // Set up ongoing IO on the connection's file descriptor
            on<IO>(connection.fd, IO::READ | IO::CLOSE).then([this](IO::Event event) {
                if (event.events & IO::READ) {
                    char buffer[1024];
                    ssize_t bytes = ::recv(event.fd, buffer, sizeof(buffer), 0);
                    if (bytes > 0) {
                        std::string msg(buffer, bytes);
                        log("Received:", msg);

                        // Echo back
                        ::send(event.fd, buffer, bytes, 0);
                    }
                }
                if (event.events & IO::CLOSE) {
                    log("Client disconnected");
                    ::close(event.fd);
                }
            });
        });
    }
};
```

### Key Points

- `on<TCP>(port)` triggers once per new connection
- The `connection.fd` is a raw file descriptor — use `IO` to monitor reads/writes
- You are responsible for reading, writing, and closing the socket
- Omit the port argument to bind to an automatically assigned port (returned from `bind`)

### Binding to a Specific Interface

```cpp
// Listen on a specific address
auto [port, fd] = on<TCP>(9000, "192.168.1.100").then(/* ... */);
```

## UDP

### Receiving Unicast Packets

```cpp
class UDPReceiver : public NUClear::Reactor {
public:
    UDPReceiver(std::unique_ptr<NUClear::Environment> environment)
        : Reactor(std::move(environment)) {

        on<UDP>(5000).then([this](const UDP::Packet& packet) {
            if (packet) {
                log("Received", packet.payload.size(), "bytes from port",
                    ntohs(packet.remote.ipv4.sin_port));
            }
        });
    }
};
```

### Receiving Broadcast Packets

```cpp
on<UDP::Broadcast>(5000).then([this](const UDP::Packet& packet) {
    log("Broadcast received:", packet.payload.size(), "bytes");
});
```

### Receiving Multicast Packets

```cpp
on<UDP::Multicast>("239.0.0.1", 5000).then([this](const UDP::Packet& packet) {
    log("Multicast received:", packet.payload.size(), "bytes");
});
```

### Sending UDP Packets

Use `emit<Scope::UDP>` to send serialized data as a UDP datagram:

```cpp
auto data = std::make_unique<MyMessage>(/* ... */);

// Send to a specific address and port
emit<Scope::UDP>(data, "192.168.1.50", 5000);

// Send from a specific local address/port
emit<Scope::UDP>(data, "192.168.1.50", 5000, "192.168.1.1", 6000);
```

The data type must be serializable (same rules as `Network<T>`).

## Complete Example: UDP Echo Server

```cpp
#include "nuclear"

struct EchoRequest {
    std::array<char, 256> message;
    uint32_t length;
};

class UDPEcho : public NUClear::Reactor {
public:
    UDPEcho(std::unique_ptr<NUClear::Environment> environment)
        : Reactor(std::move(environment)) {

        on<UDP>(7000).then([this](const UDP::Packet& packet) {
            if (packet) {
                log("Echo:", packet.payload.size(), "bytes back to sender");

                // Build a response to send back
                auto response = std::make_unique<std::vector<uint8_t>>(packet.payload);

                // Use raw socket to reply (UDP::Packet gives us the remote address)
                util::FileDescriptor fd = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
                ::sendto(fd,
                         packet.payload.data(),
                         packet.payload.size(),
                         0,
                         &packet.remote.sock,
                         packet.remote.size());
            }
        });
    }
};
```

## UDP Packet Structure

The `UDP::Packet` received in callbacks contains:

| Field     | Type                    | Description                          |
| --------- | ----------------------- | ------------------------------------ |
| `valid`   | `bool`                  | Whether the packet contains data     |
| `local`   | `sock_t`               | Local address the packet arrived on  |
| `remote`  | `sock_t`               | Remote address of the sender         |
| `payload` | `std::vector<uint8_t>` | Raw bytes of the received datagram   |
