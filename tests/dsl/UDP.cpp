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

namespace {

constexpr unsigned short PORT = 40000;
const std::string TEST_STRING = "Hello UDP World!";
bool received_a               = false;
bool received_b               = false;

struct Message {};

class TestReactor : public NUClear::Reactor {
public:
    TestReactor(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {

        // Known port
        on<UDP>(PORT).then([this](const UDP::Packet& packet) {
            // Check that the data we received is correct
            REQUIRE(packet.remote.address == INADDR_LOOPBACK);
            REQUIRE(packet.payload.size() == TEST_STRING.size());
            REQUIRE(std::memcmp(packet.payload.data(), TEST_STRING.data(), TEST_STRING.size()) == 0);

            received_a = true;
            if (received_a && received_b) {
                // Shutdown we are done with the test
                powerplant.shutdown();
            }
        });

        // Unknown port
        in_port_t bound_port;
        std::tie(std::ignore, bound_port, std::ignore) = on<UDP>().then([this](const UDP::Packet& packet) {
            // Check that the data we received is correct
            REQUIRE(packet.remote.address == INADDR_LOOPBACK);
            REQUIRE(packet.payload.size() == TEST_STRING.size());
            REQUIRE(std::memcmp(packet.payload.data(), TEST_STRING.data(), TEST_STRING.size()) == 0);

            received_b = true;
            if (received_a && received_b) {
                // Shutdown we are done with the test
                powerplant.shutdown();
            }
        });

        // Send a test for a known port
        // does not need to include the port in the lambda capture.  This is a global variable to the unit test, so
        // the function will have access to it.
        on<Trigger<Message>>().then(
            [this] { emit<Scope::UDP>(std::make_unique<std::string>(TEST_STRING), INADDR_LOOPBACK, PORT); });


        // Send a test for an unknown port
        // needs to include the bound_port in the lambda capture, so that the function will have access to bound_port.
        on<Trigger<Message>>().then([this, bound_port] {
            // Emit our UDP message
            emit<Scope::UDP>(std::make_unique<std::string>(TEST_STRING), INADDR_LOOPBACK, bound_port);
        });

        on<Startup>().then([this] {
            // Emit a message just so it will be when everything is running
            emit(std::make_unique<Message>());
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
}
