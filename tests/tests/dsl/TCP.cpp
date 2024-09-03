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
    KNOWN_V4_PORT = 40010,
    KNOWN_V6_PORT = 40011,
};

enum TestType : uint8_t {
    V4_KNOWN,
    V4_EPHEMERAL,
    V6_KNOWN,
    V6_EPHEMERAL,
};
std::vector<TestType> active_tests;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

in_port_t v4_port = 0;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
in_port_t v6_port = 0;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

struct TestConnection {
    TestConnection(std::string name, std::string address, in_port_t port)
        : name(std::move(name)), address(std::move(address)), port(port) {}
    std::string name;
    std::string address;
    in_port_t port;
};

struct Finished {};

class TestReactor : public test_util::TestBase<TestReactor> {
public:
    void handle_data(const std::string& name, const IO::Event& event) {
        // We have data to read
        if ((event.events & IO::READ) != 0) {

            // Read into the buffer
            std::array<char, 1024> buff{};
            const ssize_t len = ::recv(event.fd, buff.data(), socklen_t(buff.size()), 0);
            if (len != 0) {
                events.push_back(name + " received: " + std::string(buff.data(), len));
                ::send(event.fd, buff.data(), socklen_t(len), 0);
            }
        }

        if ((event.events & IO::CLOSE) != 0) {
            events.push_back(name + " closed");
            emit(std::make_unique<Finished>());
        }
    }

    TestReactor(std::unique_ptr<NUClear::Environment> environment)
        : TestBase(std::move(environment), false, std::chrono::seconds(2)) {

        for (const auto& t : active_tests) {
            switch (t) {
                // Bind to IPv4 and a known port
                case V4_KNOWN: {
                    on<TCP>(KNOWN_V4_PORT).then([this](const TCP::Connection& connection) {
                        on<IO>(connection.fd, IO::READ | IO::CLOSE).then([this](IO::Event event) {
                            handle_data("v4 Known", event);
                        });
                    });
                } break;
                // Bind to IPv4 an unknown port and get the port number
                case V4_EPHEMERAL: {
                    auto v4 = on<TCP>().then([this](const TCP::Connection& connection) {
                        on<IO>(connection.fd, IO::READ | IO::CLOSE).then([this](IO::Event event) {
                            handle_data("v4 Ephemeral", event);
                        });
                    });
                    v4_port = std::get<1>(v4);
                } break;

                // Bind to IPv6 and a known port
                case V6_KNOWN: {
                    on<TCP>(KNOWN_V6_PORT, "::").then([this](const TCP::Connection& connection) {
                        on<IO>(connection.fd, IO::READ | IO::CLOSE).then([this](IO::Event event) {
                            handle_data("v6 Known", event);
                        });
                    });
                } break;

                // Bind to IPv6 an unknown port and get the port number
                case V6_EPHEMERAL: {
                    auto v6 = on<TCP>(0, "::").then([this](const TCP::Connection& connection) {
                        on<IO>(connection.fd, IO::READ | IO::CLOSE).then([this](IO::Event event) {
                            handle_data("v6 Ephemeral", event);
                        });
                    });
                    v6_port = std::get<1>(v6);
                } break;
            }
        }

        // Send a test message to the known port
        on<Trigger<TestConnection>, Sync<TestReactor>>().then([](const TestConnection& target) {
            // Resolve the target address
            const NUClear::util::network::sock_t address = NUClear::util::network::resolve(target.address, target.port);

            // Open a random socket
            NUClear::util::FileDescriptor fd(::socket(address.sock.sa_family, SOCK_STREAM, IPPROTO_TCP),
                                             [](NUClear::fd_t fd) { ::shutdown(fd, SHUT_RDWR); });

            if (!fd.valid()) {
                throw std::runtime_error("Failed to create socket");
            }

            // Set a timeout so we don't hang forever if something goes wrong
#ifdef _WIN32
            DWORD timeout = 500;
#else
            timeval timeout{};
            timeout.tv_sec  = 0;
            timeout.tv_usec = 500000;
#endif
            ::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));

            // Connect to ourself
            if (::connect(fd, &address.sock, address.size()) != 0) {
                throw std::runtime_error("Failed to connect to socket");
            }

            // Write on our socket
            events.push_back(target.name + " sending");
            ::send(fd, target.name.data(), socklen_t(target.name.size()), 0);

            // Receive the echo
            std::array<char, 1024> buff{};
            const ssize_t recv = ::recv(fd, buff.data(), socklen_t(target.name.size()), 0);
            if (recv <= 1) {
                events.push_back(target.name + " failed to receive echo");
            }
            else {
                events.push_back(target.name + " echoed: " + std::string(buff.data(), recv));
            }
        });

        on<Trigger<Finished>, Sync<TestReactor>>().then([this](const Finished&) {
            if (test_no < active_tests.size()) {
                switch (active_tests[test_no++]) {
                    case V4_KNOWN:
                        emit(std::make_unique<TestConnection>("v4 Known", "127.0.0.1", KNOWN_V4_PORT));
                        break;
                    case V4_EPHEMERAL:
                        emit(std::make_unique<TestConnection>("v4 Ephemeral", "127.0.0.1", v4_port));
                        break;
                    case V6_KNOWN: emit(std::make_unique<TestConnection>("v6 Known", "::1", KNOWN_V6_PORT)); break;
                    case V6_EPHEMERAL: emit(std::make_unique<TestConnection>("v6 Ephemeral", "::1", v6_port)); break;
                    default:
                        events.push_back("Unexpected test");
                        powerplant.shutdown();
                        break;
                }
            }
            else {
                events.push_back("Finishing Test");
                powerplant.shutdown();
            }
        });

        on<Startup>().then([this] {
            // Start the first test by emitting a "finished" event
            emit(std::make_unique<Finished>());
        });
    }

private:
    size_t test_no = 0;
    NUClear::util::FileDescriptor known_port_fd;
    NUClear::util::FileDescriptor ephemeral_port_fd;
};


TEST_CASE("Testing listening for TCP connections and receiving data messages", "[api][network][tcp]") {

    // First work out what tests will be active
    active_tests.push_back(V4_KNOWN);
    active_tests.push_back(V4_EPHEMERAL);
    if (test_util::has_ipv6()) {
        active_tests.push_back(V6_KNOWN);
        active_tests.push_back(V6_EPHEMERAL);
    }

    NUClear::Configuration config;
    config.default_pool_concurrency = 2;
    NUClear::PowerPlant plant(config);
    test_util::add_tracing(plant);
    plant.install<NUClear::extension::IOController>();
    plant.install<TestReactor>();
    plant.start();

    // Get the results for the tests we expect
    std::vector<std::string> expected;
    for (const auto& t : active_tests) {
        switch (t) {
            case V4_KNOWN:
                expected.push_back("v4 Known sending");
                expected.push_back("v4 Known received: v4 Known");
                expected.push_back("v4 Known echoed: v4 Known");
                expected.push_back("v4 Known closed");
                break;
            case V4_EPHEMERAL:
                expected.push_back("v4 Ephemeral sending");
                expected.push_back("v4 Ephemeral received: v4 Ephemeral");
                expected.push_back("v4 Ephemeral echoed: v4 Ephemeral");
                expected.push_back("v4 Ephemeral closed");
                break;
            case V6_KNOWN:
                expected.push_back("v6 Known sending");
                expected.push_back("v6 Known received: v6 Known");
                expected.push_back("v6 Known echoed: v6 Known");
                expected.push_back("v6 Known closed");
                break;
            case V6_EPHEMERAL:
                expected.push_back("v6 Ephemeral sending");
                expected.push_back("v6 Ephemeral received: v6 Ephemeral");
                expected.push_back("v6 Ephemeral echoed: v6 Ephemeral");
                expected.push_back("v6 Ephemeral closed");
                break;
        }
    }
    expected.push_back("Finishing Test");

    // Make an info print the diff in an easy to read way if we fail
    INFO(test_util::diff_string(expected, events));

    // Check the events fired in order and only those events
    REQUIRE(events == expected);
}
