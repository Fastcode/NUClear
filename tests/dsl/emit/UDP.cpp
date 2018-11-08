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

// Anonymous namespace to keep everything file local
namespace {

int received_messages = 0;

class TestReactor : public NUClear::Reactor {
public:
    in_port_t bound_port = 0;

    TestReactor(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {
        emit<Scope::INITIALIZE>(std::make_unique<int>(5));

        std::tie(std::ignore, bound_port, std::ignore) = on<UDP>().then([this](const UDP::Packet& packet) {
            ++received_messages;

            switch (packet.payload.front()) {
                case 'a':
                case 'b':
                    REQUIRE(packet.remote.address == INADDR_LOOPBACK);
                    REQUIRE(packet.local.address == INADDR_LOOPBACK);
                    REQUIRE(packet.local.port == bound_port);
                    break;
                case 'c':
                    REQUIRE(packet.remote.address == INADDR_LOOPBACK);
                    REQUIRE(packet.remote.port == 12345);
                    REQUIRE(packet.local.address == INADDR_LOOPBACK);
                    REQUIRE(packet.local.port == bound_port);
                    break;
                case 'd':
                    REQUIRE(packet.remote.address == INADDR_LOOPBACK);
                    REQUIRE(packet.remote.port == 54321);
                    REQUIRE(packet.local.address == INADDR_LOOPBACK);
                    REQUIRE(packet.local.port == bound_port);
                    break;
            }

            if (received_messages == 4) { powerplant.shutdown(); }
        });

        on<Startup>().then([this] {
            // Send using a string
            emit<Scope::UDP>(std::make_unique<char>('a'), "127.0.0.1", bound_port);
            emit<Scope::UDP>(std::make_unique<char>('b'), INADDR_LOOPBACK, bound_port);
            emit<Scope::UDP>(std::make_unique<char>('c'), "127.0.0.1", bound_port, INADDR_ANY, in_port_t(12345));
            emit<Scope::UDP>(std::make_unique<char>('d'), INADDR_LOOPBACK, bound_port, INADDR_ANY, in_port_t(54321));
        });
    }
};
}  // namespace

TEST_CASE("Testing UDP emits work correctly", "[api][emit][udp]") {
    NUClear::PowerPlant::Configuration config;
    config.thread_count = 1;
    NUClear::PowerPlant plant(config);
    plant.install<TestReactor>();

    plant.start();
}
