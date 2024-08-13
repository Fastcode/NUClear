/*
 * MIT License
 *
 * Copyright (c) 2015 NUClear Contributors
 *
 * This file is part of the NUClear codebase.
 * See https://github.com/Fastcode/NUClear for further info.
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

#include "NetworkController.hpp"

#include "../message/NetworkEvent.hpp"

namespace NUClear {
namespace extension {

    using NetworkListen        = dsl::word::NetworkListen;
    using NetworkEmit          = dsl::word::emit::NetworkEmit;
    using NetworkConfiguration = message::NetworkConfiguration;
    using Unbind               = dsl::operation::Unbind<NetworkListen>;
    struct ProcessNetwork {};

    NetworkController::NetworkController(std::unique_ptr<NUClear::Environment> environment)
        : Reactor(std::move(environment)) {

        // Set our function callback
        network.set_packet_callback([this](const network::NUClearNetwork::NetworkTarget& remote,
                                           const uint64_t& hash,
                                           const bool& reliable,
                                           std::vector<uint8_t>&& payload) {
            // Construct our NetworkSource information
            dsl::word::NetworkSource src{remote.name, remote.target, reliable};

            // Move the payload in as we are stealing it
            std::vector<uint8_t> p(std::move(payload));

            // Store in our thread local cache
            dsl::store::ThreadStore<std::vector<uint8_t>>::value     = &p;
            dsl::store::ThreadStore<dsl::word::NetworkSource>::value = &src;

            /* Mutex Scope */ {
                // Lock our reaction mutex
                const std::lock_guard<std::mutex> lock(reaction_mutex);

                // Find interested reactions
                auto rs = reactions.equal_range(hash);

                // Execute on our interested reactions
                for (auto it = rs.first; it != rs.second; ++it) {
                    powerplant.submit(it->second->get_task());
                }
            }

            // Clear our cache
            dsl::store::ThreadStore<std::vector<uint8_t>>::value     = nullptr;
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
            const std::chrono::steady_clock::duration emit_offset = t - std::chrono::steady_clock::now();
            emit<Scope::DELAY>(std::make_unique<ProcessNetwork>(),
                               std::chrono::duration_cast<NUClear::clock::duration>(emit_offset));
        });

        // Start listening for a new network type
        on<Trigger<NetworkListen>>().then("Network Bind", [this](const NetworkListen& l) {
            // Lock our reaction mutex
            const std::lock_guard<std::mutex> lock(reaction_mutex);

            // Insert our new reaction
            reactions.insert(std::make_pair(l.hash, l.reaction));
        });

        // Stop listening for a network type
        on<Trigger<Unbind>>().then("Network Unbind", [this](const Unbind& unbind) {
            // Lock our reaction mutex
            const std::lock_guard<std::mutex> lock(reaction_mutex);

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
            if (process_handle) {
                process_handle.unbind();
            }

            // Unbind all our listen handles
            if (!listen_handles.empty()) {
                for (auto& h : listen_handles) {
                    h.unbind();
                }
                listen_handles.clear();
            }

            // Name becomes hostname by default if not set
            const std::string name = config.name.empty() ? util::get_hostname() : config.name;

            // Reset our network using this configuration
            network.reset(name, config.announce_address, config.announce_port, config.bind_address, config.mtu);

            // Execution handle
            process_handle = on<Trigger<ProcessNetwork>>().then("Network processing", [this] { network.process(); });

            for (auto& fd : network.listen_fds()) {
                listen_handles.push_back(on<IO>(fd, IO::READ).then("Packet", [this] { network.process(); }));
            }
        });
    }

}  // namespace extension
}  // namespace NUClear
