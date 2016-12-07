/*
 * Copyright (C) 2013-2016 Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
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

#include "nuclear"

namespace {

    constexpr unsigned short port = 40000;
    const std::string testString = "Hello UDP World!";
    bool receivedA = false;
    bool receivedB = false;

    struct Message {
    };

    class TestReactor : public NUClear::Reactor {
    public:
        TestReactor(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {

            // Known port
            on<UDP>(port).then([this](const UDP::Packet& packet) {

                // Check that the data we received is correct
                REQUIRE(packet.remote.address == INADDR_LOOPBACK);
                REQUIRE(packet.data.size() == testString.size());
                REQUIRE(std::memcmp(packet.data.data(), testString.data(), testString.size()) == 0);

                receivedA = true;
                if (receivedA && receivedB) {
                    // Shutdown we are done with the test
                    powerplant.shutdown();
                }
            });

            // Unknown port
            in_port_t boundPort;
            std::tie(std::ignore, boundPort, std::ignore) = on<UDP>().then([this](const UDP::Packet& packet) {

                // Check that the data we received is correct
                REQUIRE(packet.remote.address == INADDR_LOOPBACK);
                REQUIRE(packet.data.size() == testString.size());
                REQUIRE(std::memcmp(packet.data.data(), testString.data(), testString.size()) == 0);

                receivedB = true;
                if (receivedA && receivedB) {
                    // Shutdown we are done with the test
                    powerplant.shutdown();
                }
            });

            // Send a test for a known port
            on<Trigger<Message>>().then([this] {

                emit<Scope::UDP>(std::make_unique<std::string>(testString), INADDR_LOOPBACK, port);
            });


            // Send a test for an unknown port
            on<Trigger<Message>>().then([this, boundPort] {

                // Emit our UDP message
                emit<Scope::UDP>(std::make_unique<std::string>(testString), INADDR_LOOPBACK, boundPort);
            });

            on<Startup>().then([this] {

                // Emit a message just so it will be when everything is running
                emit(std::make_unique<Message>());
            });

        }
    };
}

TEST_CASE("Testing sending and receiving of UDP messages", "[api][network][udp]") {

    NUClear::PowerPlant::Configuration config;
    config.threadCount = 1;
    NUClear::PowerPlant plant(config);
    plant.install<TestReactor>();

    plant.start();
}
