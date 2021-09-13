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

constexpr in_port_t PORT            = 40002;
const std::string TEST_STRING       = "Hello UDP Multicast World!";
const std::string MULTICAST_ADDRESS = "230.12.3.21";
std::size_t count                   = 0;
std::size_t num_addresses           = 0;

struct Message {};

class TestReactor : public NUClear::Reactor {
public:
    bool shutdown_flag = false;

    TestReactor(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {

        // Terminates the test if it takes too long - longer than 200 ms since this reaction first runs
        on<Every<200, std::chrono::milliseconds>>().then([this]() {
            if (shutdown_flag) { powerplant.shutdown(); }
            shutdown_flag = true;
        });

        // Known port
        on<UDP::Multicast>(MULTICAST_ADDRESS, PORT).then([this](const UDP::Packet& packet) {
            ++count;
            // Check that the data we received is correct
            REQUIRE(packet.payload.size() == TEST_STRING.size());
            REQUIRE(std::memcmp(packet.payload.data(), TEST_STRING.data(), TEST_STRING.size()) == 0);

            // Shutdown if we have succeeded
            if (count >= num_addresses) { powerplant.shutdown(); }
        });

        // Test with known port
        on<Trigger<Message>>().then([this] {
            // Get all the network interfaces
            auto interfaces = NUClear::util::network::get_interfaces();

            std::vector<in_addr_t> addresses;

            for (auto& iface : interfaces) {
                // We send on multicast capable addresses
                if (iface.broadcast.sock.sa_family == AF_INET && iface.flags.multicast) {
                    auto& i = *reinterpret_cast<sockaddr_in*>(&iface.ip);
                    auto& b = *reinterpret_cast<sockaddr_in*>(&iface.broadcast);

                    // Two broadcast ips that are the same are probably on the same network so ignore those
                    if (std::find(std::begin(addresses), std::end(addresses), ntohl(b.sin_addr.s_addr))
                        == std::end(addresses)) {
                        addresses.push_back(ntohl(i.sin_addr.s_addr));
                    }
                }
            }

            num_addresses = addresses.size();

            for (auto& ad : addresses) {

                // Send our message to that broadcast address
                emit<Scope::UDP>(std::make_unique<std::string>(TEST_STRING), MULTICAST_ADDRESS, PORT, ad, in_port_t(0));
            }
        });

        on<Startup>().then([this] {
            // Emit a message to start the test
            emit(std::make_unique<Message>());
        });
    }
};
}  // namespace

TEST_CASE("Testing sending and receiving of UDP Multicast messages with a known port",
          "[api][network][udp][multicast][known_port]") {

    NUClear::PowerPlant::Configuration config;
    config.thread_count = 1;
    NUClear::PowerPlant plant(config);
    plant.install<TestReactor>();

    plant.start();

    REQUIRE(count == num_addresses);
}
