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

constexpr in_port_t PORT = 40009;

// NOLINTNEXTLINE(cert-err58-cpp,cppcoreguidelines-avoid-non-const-global-variables)
const std::string TEST_STRING = "Hello TCP World!";

struct TestConnection {
    TestConnection(std::string name, in_port_t port) : name(std::move(name)), port(port) {}
    std::string name;
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
            const ssize_t len = ::recv(event.fd, buff.data(), socklen_t(TEST_STRING.size()), 0);
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

        // Bind to a known port
        on<TCP>(PORT).then([this](const TCP::Connection& connection) {
            on<IO>(connection.fd, IO::READ | IO::CLOSE).then([this](IO::Event event) {
                handle_data("Known Port", event);
            });
        });

        // Bind to an unknown port and get the port number
        in_port_t ephemeral_port                           = 0;
        std::tie(std::ignore, ephemeral_port, std::ignore) = on<TCP>().then([this](const TCP::Connection& connection) {
            on<IO>(connection.fd, IO::READ | IO::CLOSE).then([this](IO::Event event) {
                handle_data("Ephemeral Port", event);
            });
        });

        // Send a test message to the known port
        on<Trigger<TestConnection>, Sync<TestReactor>>().then([](const TestConnection& target) {
            // Open a random socket
            NUClear::util::FileDescriptor fd(::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP),
                                             [](NUClear::fd_t fd) { ::shutdown(fd, SHUT_RDWR); });

            if (!fd.valid()) {
                throw std::runtime_error("Failed to create socket");
            }

            // Our address to our local connection
            sockaddr_in address{};
            address.sin_family      = AF_INET;
            address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            address.sin_port        = htons(target.port);

            // Connect to ourself
            if (::connect(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
                throw std::runtime_error("Failed to connect to socket");
            }

            // Write on our socket
            events.push_back(target.name + " sending");
            ::send(fd, TEST_STRING.data(), socklen_t(TEST_STRING.size()), 0);

            // Receive the echo
            std::array<char, 1024> buff{};
            const ssize_t recv = ::recv(fd, buff.data(), socklen_t(TEST_STRING.size()), 0);
            events.push_back(target.name + " echoed: " + std::string(buff.data(), recv));
        });

        on<Trigger<Finished>, Sync<TestReactor>>().then([this, ephemeral_port](const Finished& test) {
            if (test.name == "Known Port") {
                emit(std::make_unique<TestConnection>("Ephemeral Port", ephemeral_port));
            }
            else if (test.name == "Ephemeral Port") {
                events.push_back("Finishing Test");
                powerplant.shutdown();
            }
        });

        on<Startup>().then([this] {
            // Start the first test
            emit(std::make_unique<TestConnection>("Known Port", PORT));
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
        "Known Port sending",
        "Known Port received: Hello TCP World!",
        "Known Port echoed: Hello TCP World!",
        "Known Port closed",
        "Ephemeral Port sending",
        "Ephemeral Port received: Hello TCP World!",
        "Ephemeral Port echoed: Hello TCP World!",
        "Ephemeral Port closed",
        "Finishing Test",
    };

    // Make an info print the diff in an easy to read way if we fail
    INFO(test_util::diff_string(expected, events));

    // Check the events fired in order and only those events
    REQUIRE(events == expected);
}
