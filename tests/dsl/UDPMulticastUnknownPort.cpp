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

const std::string TEST_STRING       = "Hello UDP Multicast World!";  // NOLINT(cert-err58-cpp)
const std::string MULTICAST_ADDRESS = "230.12.3.22";                 // NOLINT(cert-err58-cpp)
std::size_t count                   = 0;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
std::size_t num_addresses           = 0;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

struct Message {};

class TestReactor : public NUClear::Reactor {
public:
    bool shutdown_flag = false;

    TestReactor(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {

        // Terminates the test if it takes too long - longer than 200 ms since this reaction first runs
        on<Every<200, std::chrono::milliseconds>>().then([this]() {
            if (shutdown_flag) {
                powerplant.shutdown();
            }
            shutdown_flag = true;
        });

        // Unknown port
        in_port_t bound_port = 0;
        std::tie(std::ignore, bound_port, std::ignore) =
            on<UDP::Multicast>(MULTICAST_ADDRESS).then([this](const UDP::Packet& packet) {
                ++count;
                // Check that the data we received is correct
                REQUIRE(packet.payload.size() == TEST_STRING.size());
                REQUIRE(std::memcmp(packet.payload.data(), TEST_STRING.data(), TEST_STRING.size()) == 0);

                // Shutdown if we have succeeded
                if (count >= num_addresses) {
                    powerplant.shutdown();
                }
            });

        // Test with port given to us
        on<Trigger<Message>>().then([this, bound_port] {
            // Get all the network interfaces
            // Send our message to that broadcast address
            emit<Scope::UDP>(std::make_unique<std::string>(TEST_STRING), MULTICAST_ADDRESS, bound_port);
        });

        on<Startup>().then([this] {
            // Emit a message to start the test
            emit(std::make_unique<Message>());
        });
    }
};
}  // namespace

TEST_CASE("Testing sending and receiving of UDP Multicast messages with an unknown port",
          "[api][network][udp][multicast][unknown_port]") {

    NUClear::PowerPlant::Configuration config;
    config.thread_count = 1;
    NUClear::PowerPlant plant(config);
    plant.install<TestReactor>();

    plant.start();

    REQUIRE(count == 1);
}
