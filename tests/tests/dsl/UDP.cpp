/*
 * MIT License
 *
 * Copyright (c) 2015 NUClear Contributors
 *
 * This file is part of the NUClear codebase.
 * See https://github.com/Fastcode/NUClear for further info.
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

#include <catch2/catch_test_macros.hpp>
#include <nuclear>

#include "test_util/TestBase.hpp"
#include "test_util/common.hpp"
#include "test_util/has_ipv6.hpp"

/// Events that occur during the test
std::vector<std::string> events;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

enum TestPorts : in_port_t {
    UNICAST_V4   = 40000,
    UNICAST_V6   = 40001,
    BROADCAST_V4 = 40002,
    MULTICAST_V4 = 40003,
    MULTICAST_V6 = 40004,
};

const std::string IPV4_MULTICAST_ADDRESS = "230.12.3.22";        // NOLINT(cert-err58-cpp)
const std::string IPV6_MULTICAST_ADDRESS = "ff02::230:12:3:22";  // NOLINT(cert-err58-cpp)

#ifdef __APPLE__
// For the IPv6 test we need to bind to the IPv6 localhost address and send from it when using udp emit.
// This is because on OSX without a fully connected IPv6 there is no default route for IPv6 multicast packets
// (see `netstat -nr`) As a result if you don't specify an interface to use when sending and receiving IPv6 multicast
// packets the send/bind fails which makes the tests fail.
const std::string IPV6_BIND = "::1";  // NOLINT(cert-err58-cpp)
#else
const std::string IPV6_BIND = "::";  // NOLINT(cert-err58-cpp)
#endif

// Ephemeral ports that we will use
in_port_t uni_v4_port   = 0;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
in_port_t uni_v6_port   = 0;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
in_port_t broad_v4_port = 0;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
in_port_t multi_v4_port = 0;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
in_port_t multi_v6_port = 0;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

enum TestType : uint8_t {
    UNICAST_V4_KNOWN,
    UNICAST_V4_EPHEMERAL,
    UNICAST_V6_KNOWN,
    UNICAST_V6_EPHEMERAL,
    BROADCAST_V4_KNOWN,
    BROADCAST_V4_EPHEMERAL,
    MULTICAST_V4_KNOWN,
    MULTICAST_V4_EPHEMERAL,
    MULTICAST_V6_KNOWN,
    MULTICAST_V6_EPHEMERAL,
};
std::vector<TestType> active_tests;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

inline std::string get_broadcast_addr() {
    static std::string addr;

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
    std::string data;
    struct Target {
        std::string address;
        in_port_t port = 0;
    };
    Target to{};
    Target from{};
};
std::vector<SendTarget> send_targets(const std::string& type, in_port_t port) {
    std::vector<SendTarget> results;

    // Loop through the active tests and add the send targets
    // Make sure that the type we are actually after is sent last
    for (const auto& t : active_tests) {
        switch (t) {
            case UNICAST_V4_KNOWN:
            case UNICAST_V4_EPHEMERAL: {
                results.push_back(SendTarget{type + ":Uv4", {"127.0.0.1", port}, {}});
            } break;
            case UNICAST_V6_KNOWN:
            case UNICAST_V6_EPHEMERAL: {
                results.push_back(SendTarget{type + ":Uv6", {"::1", port}, {}});
            } break;
            case BROADCAST_V4_KNOWN:
            case BROADCAST_V4_EPHEMERAL: {
                results.push_back(SendTarget{type + ":Bv4", {get_broadcast_addr(), port}, {}});
            } break;
            case MULTICAST_V4_KNOWN:
            case MULTICAST_V4_EPHEMERAL: {
                results.push_back(SendTarget{type + ":Mv4", {IPV4_MULTICAST_ADDRESS, port}, {}});
            } break;
            case MULTICAST_V6_KNOWN:
            case MULTICAST_V6_EPHEMERAL: {
                results.push_back(SendTarget{type + ":Mv6", {IPV6_MULTICAST_ADDRESS, port}, {IPV6_BIND, 0}});
            } break;
        }
    }

    // remove duplicates
    results.erase(std::unique(results.begin(),
                              results.end(),
                              [](const SendTarget& a, const SendTarget& b) {
                                  return a.to.address == b.to.address && a.to.port == b.to.port && a.data == b.data
                                         && a.from.address == b.from.address && a.from.port == b.from.port;
                              }),
                  results.end());

    // Stable sort so that the type we are after is last
    std::stable_sort(results.begin(), results.end(), [](const SendTarget& /*a*/, const SendTarget& b) {
        // We want to sort such that the one we are after is last and everything else is unmodified
        // That means that every comparision except one should be false
        // This is because equality is implied if a < b == false and b < a == false
        // The only time we should return true is when b is our target (which would make a less than it)
        return b.data.substr(0, 3) == b.data.substr(5, 8);
    });

    return results;
}

struct Finished {
    Finished(std::string name) : name(std::move(name)) {}
    std::string name;
};

class TestReactor : public test_util::TestBase<TestReactor> {
private:
    void handle_data(const std::string& name, const UDP::Packet& packet) {
        const std::string data(packet.payload.begin(), packet.payload.end());

        // Convert IP address to string in dotted decimal format
        const std::string local = packet.local.address + ":" + std::to_string(packet.local.port);

        events.push_back(name + " <- " + data + " (" + local + ")");

        if (data == (name + ":" + name.substr(0, 3))) {
            emit(std::make_unique<Finished>(name));
        }
    }

public:
    TestReactor(std::unique_ptr<NUClear::Environment> environment) : TestBase(std::move(environment), false) {

        for (const auto& t : active_tests) {
            switch (t) {
                case UNICAST_V4_KNOWN: {
                    on<UDP>(UNICAST_V4).then([this](const UDP::Packet& packet) {  //
                        handle_data("Uv4K", packet);
                    });
                } break;

                    // IPv4 Unicast Ephemeral port
                case UNICAST_V4_EPHEMERAL: {
                    auto uni_v4 = on<UDP>().then([this](const UDP::Packet& packet) {  //
                        handle_data("Uv4E", packet);
                    });
                    uni_v4_port = std::get<1>(uni_v4);
                } break;

                // IPv6 Unicast Known port
                case UNICAST_V6_KNOWN: {
                    on<UDP>(UNICAST_V6, "::").then([this](const UDP::Packet& packet) {  //
                        handle_data("Uv6K", packet);
                    });
                } break;

                // IPv6 Unicast Ephemeral port
                case UNICAST_V6_EPHEMERAL: {
                    auto uni_v6 = on<UDP>(0, "::").then([this](const UDP::Packet& packet) {  //
                        handle_data("Uv6E", packet);
                    });
                    uni_v6_port = std::get<1>(uni_v6);
                } break;

                // IPv4 Broadcast Known port
                case BROADCAST_V4_KNOWN: {
                    on<UDP::Broadcast>(BROADCAST_V4).then([this](const UDP::Packet& packet) {  //
                        handle_data("Bv4K", packet);
                    });
                } break;

                // IPv4 Broadcast Ephemeral port
                case BROADCAST_V4_EPHEMERAL: {
                    auto broad_v4 = on<UDP::Broadcast>().then([this](const UDP::Packet& packet) {  //
                        handle_data("Bv4E", packet);
                    });
                    broad_v4_port = std::get<1>(broad_v4);
                } break;

                // No such thing as broadcast in IPv6

                // IPv4 Multicast Known port
                case MULTICAST_V4_KNOWN: {
                    on<UDP::Multicast>(IPV4_MULTICAST_ADDRESS, MULTICAST_V4).then([this](const UDP::Packet& packet) {
                        handle_data("Mv4K", packet);
                    });
                } break;

                // IPv4 Multicast Ephemeral port
                case MULTICAST_V4_EPHEMERAL: {
                    auto multi_v4 = on<UDP::Multicast>(IPV4_MULTICAST_ADDRESS).then([this](const UDP::Packet& packet) {
                        handle_data("Mv4E", packet);
                    });
                    multi_v4_port = std::get<1>(multi_v4);
                } break;

                // IPv6 Multicast Known port
                case MULTICAST_V6_KNOWN: {
                    on<UDP::Multicast>(IPV6_MULTICAST_ADDRESS, MULTICAST_V6, IPV6_BIND)
                        .then([this](const UDP::Packet& packet) { handle_data("Mv6K", packet); });
                } break;

                // IPv6 Multicast Ephemeral port
                case MULTICAST_V6_EPHEMERAL: {
                    auto multi_v6 = on<UDP::Multicast>(IPV6_MULTICAST_ADDRESS, 0, IPV6_BIND)
                                        .then([this](const UDP::Packet& packet) { handle_data("Mv6E", packet); });
                    multi_v6_port = std::get<1>(multi_v6);
                } break;
            }
        }

        on<Trigger<Finished>>().then([this](const Finished&) {
            auto send_all = [this](const std::string& type, const in_port_t& port) {
                for (const auto& t : send_targets(type, port)) {
                    events.push_back(" -> " + t.to.address + ":" + std::to_string(t.to.port));
                    try {
                        emit<Scope::UDP>(std::make_unique<std::string>(t.data),
                                         t.to.address,
                                         t.to.port,
                                         t.from.address,
                                         t.from.port);
                    }
                    catch (std::exception& e) {
                        events.push_back("Exception: " + std::string(e.what()));
                    }
                }
            };

            if (test_no < active_tests.size()) {
                switch (active_tests[test_no++]) {
                    case UNICAST_V4_KNOWN: {
                        events.push_back("- Known Unicast V4 Test -");
                        send_all("Uv4K", UNICAST_V4);
                    } break;

                    case UNICAST_V4_EPHEMERAL: {
                        events.push_back("- Ephemeral Unicast V4 Test -");
                        send_all("Uv4E", uni_v4_port);
                    } break;

                    case UNICAST_V6_KNOWN: {
                        events.push_back("- Known Unicast V6 Test -");
                        send_all("Uv6K", UNICAST_V6);
                    } break;

                    case UNICAST_V6_EPHEMERAL: {
                        events.push_back("- Ephemeral Unicast V6 Test -");
                        send_all("Uv6E", uni_v6_port);
                    } break;

                    case BROADCAST_V4_KNOWN: {
                        events.push_back("- Known Broadcast V4 Test -");
                        send_all("Bv4K", BROADCAST_V4);
                    } break;

                    case BROADCAST_V4_EPHEMERAL: {
                        events.push_back("- Ephemeral Broadcast V4 Test -");
                        send_all("Bv4E", broad_v4_port);
                    } break;

                    case MULTICAST_V4_KNOWN: {
                        events.push_back("- Known Multicast V4 Test -");
                        send_all("Mv4K", MULTICAST_V4);
                    } break;

                    case MULTICAST_V4_EPHEMERAL: {
                        events.push_back("- Ephemeral Multicast V4 Test -");
                        send_all("Mv4E", multi_v4_port);
                    } break;

                    case MULTICAST_V6_KNOWN: {
                        events.push_back("- Known Multicast V6 Test -");
                        send_all("Mv6K", MULTICAST_V6);
                    } break;

                    case MULTICAST_V6_EPHEMERAL: {
                        events.push_back("- Ephemeral Multicast V6 Test -");
                        send_all("Mv6E", multi_v6_port);
                    } break;

                    default: {
                        events.push_back("Unknown test type");
                        powerplant.shutdown();
                    } break;
                }
            }
            else {
                powerplant.shutdown();
            }
        });

        on<Startup>().then([this] {
            // Start the first test by emitting a "finished" event
            emit(std::make_unique<Finished>("Startup"));
        });
    }

private:
    size_t test_no = 0;
};


TEST_CASE("Testing sending and receiving of UDP messages", "[api][network][udp]") {

    // Build up the list of active tests based on what we have available
    active_tests.push_back(UNICAST_V4_KNOWN);
    active_tests.push_back(UNICAST_V4_EPHEMERAL);
    if (test_util::has_ipv6()) {
        active_tests.push_back(UNICAST_V6_KNOWN);
        active_tests.push_back(UNICAST_V6_EPHEMERAL);
    }
    active_tests.push_back(BROADCAST_V4_KNOWN);
    active_tests.push_back(BROADCAST_V4_EPHEMERAL);
    active_tests.push_back(MULTICAST_V4_KNOWN);
    active_tests.push_back(MULTICAST_V4_EPHEMERAL);
    if (test_util::has_ipv6()) {
        active_tests.push_back(MULTICAST_V6_KNOWN);
        active_tests.push_back(MULTICAST_V6_EPHEMERAL);
    }

    NUClear::Configuration config;
    config.default_pool_concurrency = 1;
    NUClear::PowerPlant plant(config);
    test_util::add_tracing(plant);
    plant.install<NUClear::extension::IOController>();
    plant.install<TestReactor>();
    plant.start();

    std::vector<std::string> expected;
    for (const auto& t : active_tests) {
        switch (t) {
            case UNICAST_V4_KNOWN: {
                expected.push_back("- Known Unicast V4 Test -");
                for (const auto& line : send_targets("Uv4K", UNICAST_V4)) {
                    expected.push_back(" -> " + line.to.address + ":" + std::to_string(line.to.port));
                }
                expected.push_back("Uv4K <- Uv4K:Uv4 (127.0.0.1:" + std::to_string(UNICAST_V4) + ")");
            } break;

            case UNICAST_V4_EPHEMERAL: {
                expected.push_back("- Ephemeral Unicast V4 Test -");
                for (const auto& line : send_targets("Uv4E", uni_v4_port)) {
                    expected.push_back(" -> " + line.to.address + ":" + std::to_string(line.to.port));
                }
                expected.push_back("Uv4E <- Uv4E:Uv4 (127.0.0.1:" + std::to_string(uni_v4_port) + ")");
            } break;

            case UNICAST_V6_KNOWN: {
                expected.push_back("- Known Unicast V6 Test -");
                for (const auto& line : send_targets("Uv6K", UNICAST_V6)) {
                    expected.push_back(" -> " + line.to.address + ":" + std::to_string(line.to.port));
                }
                expected.push_back("Uv6K <- Uv6K:Uv6 (::1:" + std::to_string(UNICAST_V6) + ")");
            } break;

            case UNICAST_V6_EPHEMERAL: {
                expected.push_back("- Ephemeral Unicast V6 Test -");
                for (const auto& line : send_targets("Uv6E", uni_v6_port)) {
                    expected.push_back(" -> " + line.to.address + ":" + std::to_string(line.to.port));
                }
                expected.push_back("Uv6E <- Uv6E:Uv6 (::1:" + std::to_string(uni_v6_port) + ")");
            } break;

            case BROADCAST_V4_KNOWN: {
                expected.push_back("- Known Broadcast V4 Test -");
                for (const auto& line : send_targets("Bv4K", BROADCAST_V4)) {
                    expected.push_back(" -> " + line.to.address + ":" + std::to_string(line.to.port));
                }
                expected.push_back("Bv4K <- Bv4K:Bv4 (" + get_broadcast_addr() + ":" + std::to_string(BROADCAST_V4)
                                   + ")");
            } break;

            case BROADCAST_V4_EPHEMERAL: {
                expected.push_back("- Ephemeral Broadcast V4 Test -");
                for (const auto& line : send_targets("Bv4E", broad_v4_port)) {
                    expected.push_back(" -> " + line.to.address + ":" + std::to_string(line.to.port));
                }
                expected.push_back("Bv4E <- Bv4E:Bv4 (" + get_broadcast_addr() + ":" + std::to_string(broad_v4_port)
                                   + ")");
            } break;

            case MULTICAST_V4_KNOWN: {
                expected.push_back("- Known Multicast V4 Test -");
                for (const auto& line : send_targets("Mv4K", MULTICAST_V4)) {
                    expected.push_back(" -> " + line.to.address + ":" + std::to_string(line.to.port));
                }
                expected.push_back("Mv4K <- Mv4K:Mv4 (" + IPV4_MULTICAST_ADDRESS + ":" + std::to_string(MULTICAST_V4)
                                   + ")");
            } break;

            case MULTICAST_V4_EPHEMERAL: {
                expected.push_back("- Ephemeral Multicast V4 Test -");
                for (const auto& line : send_targets("Mv4E", multi_v4_port)) {
                    expected.push_back(" -> " + line.to.address + ":" + std::to_string(line.to.port));
                }
                expected.push_back("Mv4E <- Mv4E:Mv4 (" + IPV4_MULTICAST_ADDRESS + ":" + std::to_string(multi_v4_port)
                                   + ")");
            } break;

            case MULTICAST_V6_KNOWN: {
                expected.push_back("- Known Multicast V6 Test -");
                for (const auto& line : send_targets("Mv6K", MULTICAST_V6)) {
                    expected.push_back(" -> " + line.to.address + ":" + std::to_string(line.to.port));
                }
                expected.push_back("Mv6K <- Mv6K:Mv6 (" + IPV6_MULTICAST_ADDRESS + ":" + std::to_string(MULTICAST_V6)
                                   + ")");
            } break;

            case MULTICAST_V6_EPHEMERAL: {
                expected.push_back("- Ephemeral Multicast V6 Test -");
                for (const auto& line : send_targets("Mv6E", multi_v6_port)) {
                    expected.push_back(" -> " + line.to.address + ":" + std::to_string(line.to.port));
                }
                expected.push_back("Mv6E <- Mv6E:Mv6 (" + IPV6_MULTICAST_ADDRESS + ":" + std::to_string(multi_v6_port)
                                   + ")");
            } break;
        }
    }

    // Make an info print the diff in an easy to read way if we fail
    INFO(test_util::diff_string(expected, events));

    // Check the events fired in order and only those events
    REQUIRE(events == expected);
}
