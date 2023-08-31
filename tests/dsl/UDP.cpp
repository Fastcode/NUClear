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

const std::string IPV4_MULTICAST_ADDRESS = "230.12.3.22";        // NOLINT(cert-err58-cpp)
const std::string IPV6_MULTICAST_ADDRESS = "ff02::230:12:3:22";  // NOLINT(cert-err58-cpp)

// Ephemeral ports that we will use
in_port_t uni_v4_port   = 0;
in_port_t uni_v6_port   = 0;
in_port_t broad_v4_port = 0;
in_port_t multi_v4_port = 0;
in_port_t multi_v6_port = 0;

inline std::string get_broadcast_addr() {
    static std::string addr = "";

    if (!addr.empty()) {
        return addr;
    }

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
    addr = std::string(buff.data());
    return addr;
}

struct SendTarget {
    std::string data{};
    struct Target {
        std::string address = "";
        in_port_t port      = 0;
    };
    Target to{};
    Target from{};
};
std::vector<SendTarget> send_targets(const std::string& type, in_port_t port, bool include_target = false) {
    std::vector<SendTarget> results;

    // Make sure that the type we are actually after is sent last
    std::string t = type.substr(0, 3);
    if (type[2] == '4' && include_target == (t == "Uv4")) {
        results.push_back(SendTarget{type + ":Uv4", {"127.0.0.1", port}, {}});
    }
    if (type[2] == '4' && include_target == (t == "Bv4")) {
        results.push_back(SendTarget{type + ":Bv4", {get_broadcast_addr(), port}, {}});
    }
    if (type[2] == '4' && include_target == (t == "Mv4")) {
        results.push_back(SendTarget{type + ":Mv4", {IPV4_MULTICAST_ADDRESS, port}, {}});
    }
    if (type[2] == '6' && include_target == (t == "Uv6")) {
        results.push_back(SendTarget{type + ":Uv6", {"::1", port}, {}});
    }
    if (type[2] == '6' && include_target == (t == "Mv6")) {
        // For multicast v6 send from localhost so that it works on OSX
        results.push_back(SendTarget{type + ":Mv6", {IPV6_MULTICAST_ADDRESS, port}, {"::1", 0}});
    }
    if (!include_target) {
        auto target = send_targets(type, port, true);
        results.insert(results.end(), target.begin(), target.end());
    }
    return results;
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

        if (data == (name + ":" + name.substr(0, 3))) {
            emit(std::make_unique<Finished>(name));
        }
    }

public:
    TestReactor(std::unique_ptr<NUClear::Environment> environment) : TestBase(std::move(environment), false) {

        // IPv4 Unicast Known port
        on<UDP>(UNICAST_V4).then([this](const UDP::Packet& packet) {  //
            handle_data("Uv4K", packet);
        });

        // IPv4 Unicast Ephemeral port
        auto uni_v4 = on<UDP>().then([this](const UDP::Packet& packet) {  //
            handle_data("Uv4E", packet);
        });
        uni_v4_port = std::get<1>(uni_v4);

        // IPv6 Unicast Known port
        on<UDP>(UNICAST_V6, "::").then([this](const UDP::Packet& packet) {  //
            handle_data("Uv6K", packet);
        });

        // IPv6 Unicast Ephemeral port
        auto uni_v6 = on<UDP>(0, "::").then([this](const UDP::Packet& packet) {  //
            handle_data("Uv6E", packet);
        });
        uni_v6_port = std::get<1>(uni_v6);

        // IPv4 Broadcast Known port
        on<UDP::Broadcast>(BROADCAST_V4).then([this](const UDP::Packet& packet) {  //
            handle_data("Bv4K", packet);
        });

        // IPv4 Broadcast Ephemeral port
        auto broad_v4 = on<UDP::Broadcast>().then([this](const UDP::Packet& packet) {  //
            handle_data("Bv4E", packet);
        });
        broad_v4_port = std::get<1>(broad_v4);

        // No such thing as broadcast in IPv6

        // IPv4 Multicast Known port
        on<UDP::Multicast>(IPV4_MULTICAST_ADDRESS, MULTICAST_V4).then([this](const UDP::Packet& packet) {
            handle_data("Mv4K", packet);
        });

        // IPv4 Multicast Ephemeral port
        auto multi_v4 = on<UDP::Multicast>(IPV4_MULTICAST_ADDRESS).then([this](const UDP::Packet& packet) {
            handle_data("Mv4E", packet);
        });
        multi_v4_port = std::get<1>(multi_v4);

        // For the IPv6 test we need to bind to the IPv6 localhost address and send from it when using udp emit
        // This is because on OSX by default there is no default route for IPv6 multicast packets (see `netstat -nr`)
        // As a result if you don't specify an interface to use when sending and receiving IPv6 multicast packets
        // the send/bind fails which makes the tests not work. Linux does not care about this extra step so it doesn't
        // break the tests

        // IPv6 Multicast Known port
        on<UDP::Multicast>(IPV6_MULTICAST_ADDRESS, MULTICAST_V6, "::1").then([this](const UDP::Packet& packet) {
            handle_data("Mv6K", packet);
        });

        // IPv6 Multicast Ephemeral port
        auto multi_v6 = on<UDP::Multicast>(IPV6_MULTICAST_ADDRESS, 0, "::1").then([this](const UDP::Packet& packet) {
            handle_data("Mv6E", packet);
        });
        multi_v6_port = std::get<1>(multi_v6);

        // Send a test message to the known port
        on<Trigger<TestUDP>>().then([this](const TestUDP& target) {
            events.push_back(" -> " + target.address + ":" + std::to_string(target.port));
            emit<Scope::UDP>(std::make_unique<std::string>(target.name), target.address, target.port);
        });

        on<Trigger<Finished>>().then([=](const Finished& test) {
            auto send_all = [this](std::string type, in_port_t port) {
                for (const auto& t : send_targets(type, port)) {
                    events.push_back(" -> " + t.to.address + ":" + std::to_string(t.to.port));
                    emit<Scope::UDP>(std::make_unique<std::string>(t.data),
                                     t.to.address,
                                     t.to.port,
                                     t.from.address,
                                     t.from.port);
                }
            };

            if (test.name == "Startup") {
                events.push_back("- Known Unicast V4 Test -");
                send_all("Uv4K", UNICAST_V4);
            }
            else if (test.name == "Uv4K") {
                events.push_back("");
                events.push_back("- Ephemeral Unicast V4 Test -");
                send_all("Uv4E", uni_v4_port);
            }
            else if (test.name == "Uv4E") {
                events.push_back("");
                events.push_back("- Known Unicast V6 Test -");
                send_all("Uv6K", UNICAST_V6);
            }
            else if (test.name == "Uv6K") {
                events.push_back("");
                events.push_back("- Ephemeral Unicast V6 Test -");
                send_all("Uv6E", uni_v6_port);
            }
            else if (test.name == "Uv6E") {
                events.push_back("");
                events.push_back("- Known Broadcast V4 Test -");
                send_all("Bv4K", BROADCAST_V4);
            }
            else if (test.name == "Bv4K") {
                events.push_back("");
                events.push_back("- Ephemeral Broadcast V4 Test -");
                send_all("Bv4E", broad_v4_port);
            }
            else if (test.name == "Bv4E") {
                events.push_back("");
                events.push_back("- Known Multicast V4 Test -");
                send_all("Mv4K", MULTICAST_V4);
            }
            else if (test.name == "Mv4K") {
                events.push_back("");
                events.push_back("- Ephemeral Multicast V4 Test -");
                send_all("Mv4E", multi_v4_port);
            }
            else if (test.name == "Mv4E") {
                events.push_back("");
                events.push_back("- Known Multicast V6 Test -");
                send_all("Mv6K", MULTICAST_V6);
            }
            else if (test.name == "Mv6K") {
                events.push_back("");
                events.push_back("- Ephemeral Multicast V6 Test -");
                send_all("Mv6E", multi_v6_port);
            }
            else if (test.name == "Mv6E") {
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

    std::vector<std::string> expected;
    expected.push_back("- Known Unicast V4 Test -");
    for (const auto& line : send_targets("Uv4K", UNICAST_V4)) {
        expected.push_back(" -> " + line.to.address + ":" + std::to_string(line.to.port));
    }
    expected.push_back("Uv4K <- Uv4K:Uv4 (127.0.0.1:" + std::to_string(UNICAST_V4) + ")");
    expected.push_back("");

    expected.push_back("- Ephemeral Unicast V4 Test -");
    for (const auto& line : send_targets("Uv4E", uni_v4_port)) {
        expected.push_back(" -> " + line.to.address + ":" + std::to_string(line.to.port));
    }
    expected.push_back("Uv4E <- Uv4E:Uv4 (127.0.0.1:" + std::to_string(uni_v4_port) + ")");
    expected.push_back("");

    expected.push_back("- Known Unicast V6 Test -");
    for (const auto& line : send_targets("Uv6K", UNICAST_V6)) {
        expected.push_back(" -> " + line.to.address + ":" + std::to_string(line.to.port));
    }
    expected.push_back("Uv6K <- Uv6K:Uv6 (::1:" + std::to_string(UNICAST_V6) + ")");
    expected.push_back("");

    expected.push_back("- Ephemeral Unicast V6 Test -");
    for (const auto& line : send_targets("Uv6E", uni_v6_port)) {
        expected.push_back(" -> " + line.to.address + ":" + std::to_string(line.to.port));
    }
    expected.push_back("Uv6E <- Uv6E:Uv6 (::1:" + std::to_string(uni_v6_port) + ")");
    expected.push_back("");

    expected.push_back("- Known Broadcast V4 Test -");
    for (const auto& line : send_targets("Bv4K", BROADCAST_V4)) {
        expected.push_back(" -> " + line.to.address + ":" + std::to_string(line.to.port));
    }
    expected.push_back("Bv4K <- Bv4K:Bv4 (" + get_broadcast_addr() + ":" + std::to_string(BROADCAST_V4) + ")");
    expected.push_back("");

    expected.push_back("- Ephemeral Broadcast V4 Test -");
    for (const auto& line : send_targets("Bv4E", broad_v4_port)) {
        expected.push_back(" -> " + line.to.address + ":" + std::to_string(line.to.port));
    }
    expected.push_back("Bv4E <- Bv4E:Bv4 (" + get_broadcast_addr() + ":" + std::to_string(broad_v4_port) + ")");
    expected.push_back("");

    expected.push_back("- Known Multicast V4 Test -");
    for (const auto& line : send_targets("Mv4K", MULTICAST_V4)) {
        expected.push_back(" -> " + line.to.address + ":" + std::to_string(line.to.port));
    }
    expected.push_back("Mv4K <- Mv4K:Mv4 (" + IPV4_MULTICAST_ADDRESS + ":" + std::to_string(MULTICAST_V4) + ")");
    expected.push_back("");

    expected.push_back("- Ephemeral Multicast V4 Test -");
    for (const auto& line : send_targets("Mv4E", multi_v4_port)) {
        expected.push_back(" -> " + line.to.address + ":" + std::to_string(line.to.port));
    }
    expected.push_back("Mv4E <- Mv4E:Mv4 (" + IPV4_MULTICAST_ADDRESS + ":" + std::to_string(multi_v4_port) + ")");
    expected.push_back("");

    expected.push_back("- Known Multicast V6 Test -");
    for (const auto& line : send_targets("Mv6K", MULTICAST_V6)) {
        expected.push_back(" -> " + line.to.address + ":" + std::to_string(line.to.port));
    }
    expected.push_back("Mv6K <- Mv6K:Mv6 (" + IPV6_MULTICAST_ADDRESS + ":" + std::to_string(MULTICAST_V6) + ")");
    expected.push_back("");

    expected.push_back("- Ephemeral Multicast V6 Test -");
    for (const auto& line : send_targets("Mv6E", multi_v6_port)) {
        expected.push_back(" -> " + line.to.address + ":" + std::to_string(line.to.port));
    }
    expected.push_back("Mv6E <- Mv6E:Mv6 (" + IPV6_MULTICAST_ADDRESS + ":" + std::to_string(multi_v6_port) + ")");

    // Make an info print the diff in an easy to read way if we fail
    INFO(test_util::diff_string(expected, events));

    // Check the events fired in order and only those events
    REQUIRE(events == expected);
}
