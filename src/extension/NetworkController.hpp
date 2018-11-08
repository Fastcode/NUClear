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

#ifndef NUCLEAR_EXTENSION_NETWORKCONTROLLER_HPP
#define NUCLEAR_EXTENSION_NETWORKCONTROLLER_HPP

#include <algorithm>
#include <cerrno>
#include <csignal>

#include "../PowerPlant.hpp"
#include "../Reactor.hpp"
#include "../util/get_hostname.hpp"
#include "network/NUClearNetwork.hpp"

namespace NUClear {
namespace extension {

    class NetworkController : public Reactor {

        using NetworkListen        = dsl::word::NetworkListen;
        using NetworkEmit          = dsl::word::emit::NetworkEmit;
        using NetworkConfiguration = message::NetworkConfiguration;
        using Unbind               = dsl::operation::Unbind<NetworkListen>;
        struct ProcessNetwork {};

    public:
        explicit NetworkController(std::unique_ptr<NUClear::Environment> environment)
            : Reactor(std::move(environment)) {

            // Set our function callback
            network.set_packet_callback([this](const network::NUClearNetwork::NetworkTarget& remote,
                                               const uint64_t& hash,
                                               const bool& reliable,
                                               std::vector<char>&& payload) {
                // Construct our NetworkSource information
                dsl::word::NetworkSource src;
                src.name     = remote.name;
                src.address  = remote.target;
                src.reliable = reliable;

                // Store in our thread local cache
                dsl::store::ThreadStore<std::vector<char>>::value        = &payload;
                dsl::store::ThreadStore<dsl::word::NetworkSource>::value = &src;

                /* Mutex Scope */ {
                    // Lock our reaction mutex
                    std::lock_guard<std::mutex> lock(reaction_mutex);

                    // Find interested reactions
                    auto rs = reactions.equal_range(hash);

                    // Execute on our interested reactions
                    for (auto it = rs.first; it != rs.second; ++it) {
                        auto task = it->second->get_task();
                        if (task) { powerplant.submit(std::move(task)); }
                    }
                }

                // Clear our cache
                dsl::store::ThreadStore<std::vector<char>>::value        = nullptr;
                dsl::store::ThreadStore<dsl::word::NetworkSource>::value = nullptr;
            });

            // Set our join callback
            network.set_join_callback([this](const network::NUClearNetwork::NetworkTarget& remote) {
                auto l     = std::make_unique<message::NetworkJoin>();
                l->name    = remote.name;
                l->address = remote.target;
                emit(l);
            });

            // Set our leave callback
            network.set_leave_callback([this](const network::NUClearNetwork::NetworkTarget& remote) {
                auto l     = std::make_unique<message::NetworkLeave>();
                l->name    = remote.name;
                l->address = remote.target;
                emit(l);
            });

            // Set our event timer callback
            network.set_next_event_callback([this](std::chrono::steady_clock::time_point t) {
                emit<Scope::DELAY>(std::make_unique<ProcessNetwork>(), t - std::chrono::steady_clock::now());
            });

            // Start listening for a new network type
            on<Trigger<NetworkListen>>().then("Network Bind", [this](const NetworkListen& l) {
                // Lock our reaction mutex
                std::lock_guard<std::mutex> lock(reaction_mutex);

                // Insert our new reaction
                reactions.insert(std::make_pair(l.hash, l.reaction));
            });

            // Stop listening for a network type
            on<Trigger<Unbind>>().then("Network Unbind", [this](const Unbind& unbind) {
                // Lock our reaction mutex
                std::lock_guard<std::mutex> lock(reaction_mutex);

                // Find and delete this reaction
                for (auto it = reactions.begin(); it != reactions.end(); ++it) {
                    if (it->second->id == unbind.id) {
                        reactions.erase(it);
                        break;
                    }
                }
            });

            on<Trigger<NetworkEmit>>().then("Network Emit", [this](const NetworkEmit& emit) {
                network.send(emit.hash, emit.payload, emit.target, emit.reliable);
            });

            on<Shutdown>().then("Shutdown Network", [this] { network.shutdown(); });

            // Configure the NUClearNetwork options
            on<Trigger<NetworkConfiguration>>().then([this](const NetworkConfiguration& config) {
                // Unbind our announce handle
                if (process_handle) { process_handle.unbind(); }

                // Unbind all our listen handles
                if (!listen_handles.empty()) {
                    for (auto& h : listen_handles) {
                        h.unbind();
                    }
                    listen_handles.clear();
                }

                // Read the new configuration
                std::string name             = config.name.empty() ? util::get_hostname() : config.name;
                std::string announce_address = config.announce_address;
                in_port_t announce_port      = config.announce_port;
                uint16_t mtu                 = config.mtu;

                // Reset our network using this configuration
                network.reset(name, announce_address, announce_port, mtu);

                // Execution handle
                process_handle =
                    on<Trigger<ProcessNetwork>>().then("Network processing", [this] { network.process(); });

                for (auto& fd : network.listen_fds()) {
                    listen_handles.push_back(on<IO>(fd, IO::READ).then("Packet", [this] { network.process(); }));
                }
            });
        }

    private:
        /// Our NUClearNetwork object that handles the networking
        network::NUClearNetwork network;

        /// The reaction that handles timed events from the network
        ReactionHandle process_handle;
        /// The reactions that listen for io
        std::vector<ReactionHandle> listen_handles;

        /// Mutex to guard the list of reactions
        std::mutex reaction_mutex;
        /// Map of type hashes to reactions that are interested in them
        std::multimap<uint64_t, std::shared_ptr<threading::Reaction>> reactions;
    };

}  // namespace extension
}  // namespace NUClear

#endif  // NUCLEAR_EXTENSION_NETWORKCONTROLLER_HPP
