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

constexpr unsigned short PORT = 40001;
const std::string TEST_STRING = "Hello UDP Broadcast World!";
std::size_t count_a           = 0;
std::size_t count_b           = 0;
std::size_t num_addresses     = 0;

struct Message {};

class TestReactor : public NUClear::Reactor {
public:
    in_port_t bound_port = 0;
    TestReactor(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {

        // Known port
        on<UDP::Broadcast>(PORT).then([this](const UDP::Packet& packet) {
            ++count_a;

            // Check that the data we received is correct
            REQUIRE(packet.payload.size() == TEST_STRING.size());
            REQUIRE(std::memcmp(packet.payload.data(), TEST_STRING.data(), TEST_STRING.size()) == 0);

            // Shutdown we are done with the test
            if (count_a == num_addresses && count_b == num_addresses) { powerplant.shutdown(); }
        });

        // Unknown port
        std::tie(std::ignore, bound_port, std::ignore) = on<UDP::Broadcast>().then([this](const UDP::Packet& packet) {
            ++count_b;

            // Check that the data we received is correct
            REQUIRE(packet.payload.size() == TEST_STRING.size());
            REQUIRE(std::memcmp(packet.payload.data(), TEST_STRING.data(), TEST_STRING.size()) == 0);

            // Shutdown we are done with the test
            if (count_a == num_addresses && count_b == num_addresses) { powerplant.shutdown(); }
        });

        on<Trigger<Message>>().then([this] {
            // Get all the network interfaces
            auto interfaces = NUClear::util::network::get_interfaces();

            std::vector<in_addr_t> addresses;

            for (auto& iface : interfaces) {
                // We send on broadcast addresses and we don't want loopback or point to point
                if (iface.broadcast.sock.sa_family == AF_INET && iface.flags.broadcast) {
                    auto& i = *reinterpret_cast<sockaddr_in*>(&iface.broadcast);

                    // Two broadcast ips that are the same are probably on the same network so ignore those
                    if (std::find(std::begin(addresses), std::end(addresses), ntohl(i.sin_addr.s_addr))
                        == std::end(addresses)) {

                        addresses.push_back(ntohl(i.sin_addr.s_addr));
                    }
                }
            }

            num_addresses = addresses.size();

            for (auto& ad : addresses) {

                // Send our message to that broadcast address
                emit<Scope::UDP>(std::make_unique<std::string>(TEST_STRING), ad, PORT);
            }
        });

        on<Trigger<Message>>().then([this] {
            // Get all the network interfaces
            auto interfaces = NUClear::util::network::get_interfaces();

            std::vector<in_addr_t> addresses;

            for (auto& iface : interfaces) {
                // We send on broadcast addresses and we don't want loopback or point to point
                if (iface.broadcast.sock.sa_family == AF_INET && iface.flags.broadcast) {
                    auto& i = *reinterpret_cast<sockaddr_in*>(&iface.broadcast);
                    // Two broadcast ips that are the same are probably on the same network so ignore those
                    if (std::find(std::begin(addresses), std::end(addresses), ntohl(i.sin_addr.s_addr))
                        == std::end(addresses)) {

                        addresses.push_back(ntohl(i.sin_addr.s_addr));
                    }
                }
            }

            num_addresses = addresses.size();

            for (auto& ad : addresses) {

                // Send our message to that broadcast address
                emit<Scope::UDP>(std::make_unique<std::string>(TEST_STRING), ad, bound_port);
            }
        });

        on<Startup>().then([this] {
            // Emit a message just so it will be when everything is running
            emit(std::make_unique<Message>());
        });
    }
};
}  // namespace

TEST_CASE("Testing sending and receiving of UDP Broadcast messages", "[api][network][udp][broadcast]") {

    NUClear::PowerPlant::Configuration config;
    config.thread_count = 1;
    NUClear::PowerPlant plant(config);
    plant.install<TestReactor>();

    plant.start();

    REQUIRE(count_a == num_addresses);
    REQUIRE(count_b == num_addresses);
}
