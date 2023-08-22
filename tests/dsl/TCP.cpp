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
    KNOWN_V4_PORT = 40010,
    KNOWN_V6_PORT = 40011,
};

struct TestConnection {
    TestConnection(std::string name, std::string address, in_port_t port)
        : name(std::move(name)), address(std::move(address)), port(port) {}
    std::string name;
    std::string address;
    in_port_t port;
};

struct Finished {
    Finished(std::string name) : name(std::move(name)) {}
    std::string name;
};

class TestReactor : public test_util::TestBase<TestReactor, 2000> {
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
            emit(std::make_unique<Finished>(name));
        }
    }

    TestReactor(std::unique_ptr<NUClear::Environment> environment) : TestBase(std::move(environment), false) {

        // Bind to IPv4 and a known port
        on<TCP>(KNOWN_V4_PORT).then([this](const TCP::Connection& connection) {
            on<IO>(connection.fd, IO::READ | IO::CLOSE).then([this](IO::Event event) {
                handle_data("v4 Known", event);
            });
        });

        // Bind to IPv4 an unknown port and get the port number
        auto v4           = on<TCP>().then([this](const TCP::Connection& connection) {
            on<IO>(connection.fd, IO::READ | IO::CLOSE).then([this](IO::Event event) {
                handle_data("v4 Ephemeral", event);
            });
        });
        in_port_t v4_port = std::get<1>(v4);

        // Bind to IPv6 and a known port
        on<TCP>(KNOWN_V6_PORT, "::1").then([this](const TCP::Connection& connection) {
            on<IO>(connection.fd, IO::READ | IO::CLOSE).then([this](IO::Event event) {
                handle_data("v6 Known", event);
            });
        });

        // Bind to IPv6 an unknown port and get the port number
        auto v6           = on<TCP>(0, "::1").then([this](const TCP::Connection& connection) {
            on<IO>(connection.fd, IO::READ | IO::CLOSE).then([this](IO::Event event) {
                handle_data("v6 Ephemeral", event);
            });
        });
        in_port_t v6_port = std::get<1>(v6);

        // Send a test message to the known port
        on<Trigger<TestConnection>, Sync<TestReactor>>().then([](const TestConnection& target) {
            // Resolve the target address
            NUClear::util::network::sock_t address = NUClear::util::network::resolve(target.address, target.port);

            // Open a random socket
            NUClear::util::FileDescriptor fd(::socket(address.sock.sa_family, SOCK_STREAM, IPPROTO_TCP),
                                             [](NUClear::fd_t fd) { ::shutdown(fd, SHUT_RDWR); });

            if (!fd.valid()) {
                throw std::runtime_error("Failed to create socket");
            }

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
            events.push_back(target.name + " echoed: " + std::string(buff.data(), recv));
        });

        on<Trigger<Finished>, Sync<TestReactor>>().then([=](const Finished& test) {
            if (test.name == "Startup") {
                emit(std::make_unique<TestConnection>("v4 Known", "127.0.0.1", KNOWN_V4_PORT));
            }
            if (test.name == "v4 Known") {
                emit(std::make_unique<TestConnection>("v4 Ephemeral", "127.0.0.1", v4_port));
            }
            else if (test.name == "v4 Ephemeral") {
                std::cout << "Testing v6 known" << std::endl;
                emit(std::make_unique<TestConnection>("v6 Known", "::1", KNOWN_V6_PORT));
            }
            else if (test.name == "v6 Known") {
                emit(std::make_unique<TestConnection>("v6 Ephemeral", "::1", v6_port));
            }
            else if (test.name == "v6 Ephemeral") {
                events.push_back("Finishing Test");
                powerplant.shutdown();
            }
        });

        on<Startup>().then([this] {
            // Start the first test by emitting a "finished" startup
            emit(std::make_unique<Finished>("Startup"));
        });
    }

private:
    NUClear::util::FileDescriptor known_port_fd;
    NUClear::util::FileDescriptor ephemeral_port_fd;
};
}  // namespace

TEST_CASE("Testing listening for TCP connections and receiving data messages", "[api][network][tcp]") {

    NUClear::PowerPlant::Configuration config;
    config.thread_count = 2;
    NUClear::PowerPlant plant(config);
    plant.install<TestReactor>();
    plant.start();

    const std::vector<std::string> expected = {
        "v4 Known sending",
        "v4 Known received: v4 Known",
        "v4 Known echoed: v4 Known",
        "v4 Known closed",
        "v4 Ephemeral sending",
        "v4 Ephemeral received: v4 Ephemeral",
        "v4 Ephemeral echoed: v4 Ephemeral",
        "v4 Ephemeral closed",
        "v6 Known sending",
        "v6 Known received: v6 Known",
        "v6 Known echoed: v6 Known",
        "v6 Known closed",
        "v6 Ephemeral sending",
        "v6 Ephemeral received: v6 Ephemeral",
        "v6 Ephemeral echoed: v6 Ephemeral",
        "v6 Ephemeral closed",
        "Finishing Test",
    };

    // Make an info print the diff in an easy to read way if we fail
    INFO(test_util::diff_string(expected, events));

    // Check the events fired in order and only those events
    REQUIRE(events == expected);
}
