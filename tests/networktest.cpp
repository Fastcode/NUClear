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

#include "nuclear"

#include <cstdlib>
#include <csignal>

namespace {
struct PerformEmits {};

using NUClear::message::NetworkJoin;
using NUClear::message::NetworkLeave;
using NUClear::message::CommandLineArguments;

class TestReactor : public NUClear::Reactor {
public:
    TestReactor(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {

        on<Trigger<NetworkJoin>, Sync<TestReactor>>().then([this](const NetworkJoin& join) {

            const sockaddr_in& sock = *reinterpret_cast<const sockaddr_in*>(&join.address);
            in_addr_t ip            = ntohl(sock.sin_addr.s_addr);
            in_port_t port          = ntohs(sock.sin_port);

            std::cout << "Connected To" << std::endl;
            std::cout << "\tName: " << join.name << std::endl;
            std::cout << "\tAddress: " << ((ip >> 24) & 0xFF) << "." << ((ip >> 16) & 0xFF) << "." << ((ip >> 8) & 0xFF)
                      << "." << ((ip >> 0) & 0xFF) << std::endl;
            std::cout << "\tPort: " << port << std::endl;

            // Send some data to our new friend

            // Emit short unreliable message
            emit<Scope::NETWORK>(std::make_unique<std::string>("Test Short Unreliable Target Message"), join.name);

            // Emit short reliable message
            emit<Scope::NETWORK>(std::make_unique<std::string>("Test Short Reliable Target Message"), join.name, true);


            // Emit long unreliable message
            emit<Scope::NETWORK>(std::make_unique<std::string>(std::numeric_limits<uint16_t>::max(), 'v'), join.name);

            // Emit long reliable message
            emit<Scope::NETWORK>(
                std::make_unique<std::string>(std::numeric_limits<uint16_t>::max(), 's'), join.name, true);
        });

        on<Trigger<NetworkLeave>, Sync<TestReactor>>().then([this](const NetworkLeave& leave) {

            const sockaddr_in& sock = *reinterpret_cast<const sockaddr_in*>(&leave.address);
            in_addr_t ip            = ntohl(sock.sin_addr.s_addr);
            in_port_t port          = ntohs(sock.sin_port);

            std::cout << "Disconnected from" << std::endl;
            std::cout << "\tName: " << leave.name << std::endl;
            std::cout << "\tAddress: " << ((ip >> 24) & 0xFF) << "." << ((ip >> 16) & 0xFF) << "." << ((ip >> 8) & 0xFF)
                      << "." << ((ip >> 0) & 0xFF) << std::endl;
            std::cout << "\tPort: " << port << std::endl;
        });

        on<Network<std::string>, Sync<TestReactor>>().then(
            [this](const NUClear::dsl::word::NetworkSource& source, const std::string& s) {

                std::cout << "Processing a message from " << source.name << std::endl;

                if (s.size() < 100) {
                    std::cout << s << std::endl;
                }
                else {
                    std::cout << s[0] << std::endl;
                }

            });

        on<Startup, With<CommandLineArguments>>().then([this](const CommandLineArguments& args) {

            auto net_config = std::make_unique<NUClear::message::NetworkConfiguration>();

            net_config->name = args.size() > 1 ? args[1] : args[0];
            ;
            net_config->multicast_group = "239.226.152.162";
            net_config->multicast_port  = 7447;

            std::cout << "Testing network with node " << net_config->name << std::endl;

            emit<Scope::DIRECT>(net_config);
            emit(std::make_unique<PerformEmits>());
        });

        on<Trigger<PerformEmits>>().then([this] {

            // Sleep for one second to let the network stabalise
            std::this_thread::sleep_for(std::chrono::seconds(1));

            // Do a series of network emits from us to test functionality

            // Emit short unreliable to all
            emit<Scope::NETWORK>(std::make_unique<std::string>("Test Short Unreliable All Message"));

            // Emit short reliable to all
            emit<Scope::NETWORK>(std::make_unique<std::string>("Test Short Reliable All Message"), true);

            // Emit long unreliable to all
            emit<Scope::NETWORK>(std::make_unique<std::string>(std::numeric_limits<uint16_t>::max(), 'u'));

            // Emit long reliable to all
            emit<Scope::NETWORK>(std::make_unique<std::string>(std::numeric_limits<uint16_t>::max(), 'r'), true);
        });
    }
};
}


int main(int argc, const char* argv[]) {
    signal(SIGINT, [] (int) { NUClear::PowerPlant::powerplant->shutdown(); });

    NUClear::PowerPlant::Configuration config;
    config.thread_count = 4;
    NUClear::PowerPlant plant(config, argc, argv);
    plant.install<TestReactor>();

    plant.start();
}
