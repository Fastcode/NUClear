/*
 * Copyright (C) 2013      Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
 *               2014-2017 Trent Houliston <trent@houliston.me>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <catch.hpp>
#include <nuclear>

#include "test_util/TestBase.hpp"

namespace {

/// @brief Events that occur during the test
std::vector<std::string> events;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

enum TestPorts {
    UNICAST_V4   = 40000,
    UNICAST_V6   = 40001,
    BROADCAST_V4 = 40002,
    MULTICAST_V4 = 40003,
    MULTICAST_V6 = 40004,
};

// Ephemeral ports that we will use
in_port_t uni_v4_port   = 0;
in_port_t uni_v6_port   = 0;
in_port_t broad_v4_port = 0;
in_port_t multi_v4_port = 0;
in_port_t multi_v6_port = 0;

constexpr in_port_t BASE_PORT            = 40000;
const std::string IPV4_MULTICAST_ADDRESS = "230.12.3.22";        // NOLINT(cert-err58-cpp)
const std::string IPV6_MULTICAST_ADDRESS = "ff02::230:12:3:22";  // NOLINT(cert-err58-cpp)

inline std::string get_broadcast_addr() {
    // Get the first IPv4 broadcast address we can find
    std::array<char, INET_ADDRSTRLEN> buff{};
    bool found = false;
    for (const auto& iface : NUClear::util::network::get_interfaces()) {
        if (iface.ip.sock.sa_family == AF_INET && iface.flags.broadcast) {
            ::inet_ntop(AF_INET, &iface.broadcast.ipv4.sin_addr, buff.data(), buff.size());
            found = true;
            break;
        }
    }
    if (!found) {
        throw std::runtime_error("No broadcast address found");
    }
    return std::string(buff.data());
}

struct TestUDP {
    TestUDP(std::string name, std::string address, in_port_t port)
        : name(std::move(name)), address(std::move(address)), port(port) {}
    std::string name;
    std::string address;
    in_port_t port;
};

struct Finished {
    Finished(std::string name) : name(std::move(name)) {}
    std::string name;
};

class TestReactor : public test_util::TestBase<TestReactor> {
private:
    void handle_data(const std::string& name, const UDP::Packet& packet) {
        std::string data(packet.payload.data(), packet.payload.size());

        // Convert IP address to string in dotted decimal format
        std::string local = packet.local.address + ":" + std::to_string(packet.local.port);

        events.push_back(name + " <- " + data + " (" + local + ")");

        if (data == name) {
            emit(std::make_unique<Finished>(name));
        }
    }

public:
    TestReactor(std::unique_ptr<NUClear::Environment> environment) : TestBase(std::move(environment), false) {

        // Get the first IPv4 broadcast address we can find
        std::string broadcast_addr = get_broadcast_addr();

        // IPv4 Unicast Known port
        on<UDP>(UNICAST_V4).then([this](const UDP::Packet& packet) {  //
            handle_data("Uv4 K", packet);
        });

        // IPv4 Unicast Ephemeral port
        auto uni_v4 = on<UDP>().then([this](const UDP::Packet& packet) {  //
            handle_data("Uv4 E", packet);
        });
        uni_v4_port = std::get<1>(uni_v4);

        // IPv6 Unicast Known port
        on<UDP>(UNICAST_V6, "::1").then([this](const UDP::Packet& packet) {  //
            handle_data("Uv6 K", packet);
        });

        // IPv6 Unicast Ephemeral port
        auto uni_v6 = on<UDP>(0, "::1").then([this](const UDP::Packet& packet) {  //
            handle_data("Uv6 E", packet);
        });
        uni_v6_port = std::get<1>(uni_v6);

        // IPv4 Broadcast Known port
        on<UDP::Broadcast>(BROADCAST_V4).then([this](const UDP::Packet& packet) {  //
            handle_data("Bv4 K", packet);
        });

        // IPv4 Broadcast Ephemeral port
        auto broad_v4 = on<UDP::Broadcast>().then([this](const UDP::Packet& packet) {  //
            handle_data("Bv4 E", packet);
        });
        broad_v4_port = std::get<1>(broad_v4);

        // No such thing as broadcast in IPv6

        // IPv4 Multicast Known port
        on<UDP::Multicast>(IPV4_MULTICAST_ADDRESS, MULTICAST_V4).then([this](const UDP::Packet& packet) {  //
            handle_data("Mv4 K", packet);
        });

        // IPv4 Multicast Ephemeral port
        auto multi_v4 = on<UDP::Multicast>(IPV4_MULTICAST_ADDRESS).then([this](const UDP::Packet& packet) {  //
            handle_data("Mv4 E", packet);
        });
        multi_v4_port = std::get<1>(multi_v4);

        // IPv6 Multicast Known port
        on<UDP::Multicast>(IPV6_MULTICAST_ADDRESS, MULTICAST_V6).then([this](const UDP::Packet& packet) {  //
            handle_data("Mv6 K", packet);
        });

        // IPv6 Multicast Ephemeral port
        auto multi_v6 = on<UDP::Multicast>(IPV6_MULTICAST_ADDRESS).then([this](const UDP::Packet& packet) {  //
            handle_data("Mv6 E", packet);
        });
        multi_v6_port = std::get<1>(multi_v6);

        // Send a test message to the known port
        on<Trigger<TestUDP>>().then([this](const TestUDP& target) {
            events.push_back(" -> " + target.address + ":" + std::to_string(target.port));
            emit<Scope::UDP>(std::make_unique<std::string>(target.name), target.address, target.port);
        });

        on<Trigger<Finished>>().then([=](const Finished& test) {
            events.push_back("--------------------");

            if (test.name == "Startup") {
                // Send some invalid packets and a valid one
                emit(std::make_unique<TestUDP>("Bv4 I", broadcast_addr, UNICAST_V4));
                emit(std::make_unique<TestUDP>("Uv6 I", "::1", UNICAST_V4));
                emit(std::make_unique<TestUDP>("Uv4 K", "127.0.0.1", UNICAST_V4));
            }
            else if (test.name == "Uv4 K") {
                // Send some invalid packets and a valid one
                emit(std::make_unique<TestUDP>("Bv4 I", broadcast_addr, uni_v4_port));
                emit(std::make_unique<TestUDP>("Uv6 I", "::1", uni_v4_port));
                emit(std::make_unique<TestUDP>("Uv4 E", "127.0.0.1", uni_v4_port));
            }
            else if (test.name == "Uv4 E") {
                emit(std::make_unique<TestUDP>("Bv4 I", broadcast_addr, UNICAST_V6));
                emit(std::make_unique<TestUDP>("Uv4 I", "127.0.0.1", UNICAST_V6));
                emit(std::make_unique<TestUDP>("Uv6 K", "::1", UNICAST_V6));
            }
            else if (test.name == "Uv6 K") {
                emit(std::make_unique<TestUDP>("Bv4 I", broadcast_addr, uni_v6_port));
                emit(std::make_unique<TestUDP>("Uv4 I", "127.0.0.1", uni_v6_port));
                emit(std::make_unique<TestUDP>("Uv6 E", "::1", uni_v6_port));
            }
            else if (test.name == "Uv6 E") {
                // Send a unicast and broadcast (only the broadcast should be received)
                emit(std::make_unique<TestUDP>("Uv4 I", "127.0.0.1", BROADCAST_V4));
                emit(std::make_unique<TestUDP>("Uv6 E", "::1", BROADCAST_V4));
                emit(std::make_unique<TestUDP>("Bv4 K", broadcast_addr, BROADCAST_V4));
            }
            else if (test.name == "Bv4 K") {
                emit(std::make_unique<TestUDP>("Uv4 I", "127.0.0.1", broad_v4_port));
                emit(std::make_unique<TestUDP>("Uv6 E", "::1", broad_v4_port));
                emit(std::make_unique<TestUDP>("Bv4 E", broadcast_addr, broad_v4_port));
            }
            else if (test.name == "Bv4 E") {
                emit(std::make_unique<TestUDP>("Uv4 I", "127.0.0.1", MULTICAST_V4));
                emit(std::make_unique<TestUDP>("Bv4 I", broadcast_addr, MULTICAST_V4));
                emit(std::make_unique<TestUDP>("Mv4 K", IPV4_MULTICAST_ADDRESS, MULTICAST_V4));
            }
            else if (test.name == "Mv4 K") {
                emit(std::make_unique<TestUDP>("Uv4 I", "127.0.0.1", multi_v4_port));
                emit(std::make_unique<TestUDP>("Bv4 I", broadcast_addr, multi_v4_port));
                emit(std::make_unique<TestUDP>("Mv4 E", IPV4_MULTICAST_ADDRESS, multi_v4_port));
            }
            else if (test.name == "Mv4 E") {
                emit(std::make_unique<TestUDP>("Uv6 I", "::1", MULTICAST_V6));
                emit(std::make_unique<TestUDP>("Mv6 K", IPV6_MULTICAST_ADDRESS, MULTICAST_V6));
            }
            else if (test.name == "Mv6 K") {
                emit(std::make_unique<TestUDP>("Uv6 I", "::1", multi_v6_port));
                emit(std::make_unique<TestUDP>("Mv6 E", IPV6_MULTICAST_ADDRESS, multi_v6_port));
            }
            else if (test.name == "Mv6 E") {
                // We are done, so stop the reactor
                powerplant.shutdown();
            }
            else {
                FAIL("Unknown test name");
            }
        });

        on<Startup>().then([this] {
            // Start the first test by emitting a "finished" event
            emit(std::make_unique<Finished>("Startup"));
        });
    }
};

}  // namespace

TEST_CASE("Testing sending and receiving of UDP messages", "[api][network][udp]") {

    NUClear::PowerPlant::Configuration config;
    config.thread_count = 1;
    NUClear::PowerPlant plant(config);
    plant.install<TestReactor>();
    plant.start();

    const std::vector<std::string> expected = {
        "--------------------",
        " -> 192.168.189.255:" + std::to_string(UNICAST_V4) + "",
        " -> ::1:" + std::to_string(UNICAST_V4) + "",
        " -> 127.0.0.1:" + std::to_string(UNICAST_V4) + "",
        "Uv4 K <- Uv4 K (127.0.0.1:" + std::to_string(UNICAST_V4) + ")",
        "--------------------",
        " -> 192.168.189.255:" + std::to_string(uni_v4_port) + "",
        " -> ::1:" + std::to_string(uni_v4_port) + "",
        " -> 127.0.0.1:" + std::to_string(uni_v4_port) + "",
        "Uv4 E <- Uv4 E (127.0.0.1:" + std::to_string(uni_v4_port) + ")",
        "--------------------",
        " -> 192.168.189.255:" + std::to_string(UNICAST_V6) + "",
        " -> 127.0.0.1:" + std::to_string(UNICAST_V6) + "",
        " -> ::1:" + std::to_string(UNICAST_V6) + "",
        "Uv6 K <- Uv6 K (::1:" + std::to_string(UNICAST_V6) + ")",
        "--------------------",
        " -> 192.168.189.255:" + std::to_string(uni_v6_port) + "",
        " -> 127.0.0.1:" + std::to_string(uni_v6_port) + "",
        " -> ::1:" + std::to_string(uni_v6_port) + "",
        "Uv6 E <- Uv6 E (::1:" + std::to_string(uni_v6_port) + ")",
        "--------------------",
        " -> 127.0.0.1:" + std::to_string(BROADCAST_V4) + "",
        " -> ::1:" + std::to_string(BROADCAST_V4) + "",
        " -> 192.168.189.255:" + std::to_string(BROADCAST_V4) + "",
        "Bv4 K <- Bv4 K (192.168.189.255:" + std::to_string(BROADCAST_V4) + ")",
        "--------------------",
        " -> 127.0.0.1:" + std::to_string(broad_v4_port) + "",
        " -> ::1:" + std::to_string(broad_v4_port) + "",
        " -> 192.168.189.255:" + std::to_string(broad_v4_port) + "",
        "Bv4 E <- Bv4 E (192.168.189.255:" + std::to_string(broad_v4_port) + ")",
        "--------------------",
        " -> 127.0.0.1:" + std::to_string(MULTICAST_V4) + "",
        " -> 192.168.189.255:" + std::to_string(MULTICAST_V4) + "",
        " -> 230.12.3.22:" + std::to_string(MULTICAST_V4) + "",
        "Mv4 K <- Mv4 K (230.12.3.22:" + std::to_string(MULTICAST_V4) + ")",
        "--------------------",
        " -> 127.0.0.1:" + std::to_string(multi_v4_port) + "",
        " -> 192.168.189.255:" + std::to_string(multi_v4_port) + "",
        " -> 230.12.3.22:" + std::to_string(multi_v4_port) + "",
        "Mv4 E <- Mv4 E (230.12.3.22:" + std::to_string(multi_v4_port) + ")",
        "--------------------",
        " -> ::1:" + std::to_string(MULTICAST_V6) + "",
        " -> ff02::230:12:3:22:" + std::to_string(MULTICAST_V6) + "",
        "Mv6 K <- Mv6 K (ff02::230:12:3:22:" + std::to_string(MULTICAST_V6) + ")",
        "--------------------",
        " -> ::1:" + std::to_string(multi_v6_port) + "",
        " -> ff02::230:12:3:22:" + std::to_string(multi_v6_port) + "",
        "Mv6 E <- Mv6 E (ff02::230:12:3:22:" + std::to_string(multi_v6_port) + ")",
        "--------------------",
    };

    // Make an info print the diff in an easy to read way if we fail
    INFO(test_util::diff_string(expected, events));

    // Check the events fired in order and only those events
    REQUIRE(events == expected);
}
