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

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "../Reactor.hpp"
#include "../dsl/operation/Unbind.hpp"
#include "../dsl/store/ThreadStore.hpp"
#include "../dsl/word/Network.hpp"
#include "../dsl/word/emit/Network.hpp"
#include "../message/NetworkConfiguration.hpp"
#include "../message/NetworkEvent.hpp"
#include "../nuclearnet/Discovery.hpp"
#include "../nuclearnet/NUClearNet.hpp"
#include "../util/get_hostname.hpp"

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
        net.set_packet_callback([this](const network::NUClearNet::sock_t& source,
                                       const std::string& peer_name,
                                       uint64_t hash,
                                       bool reliable,
                                       std::vector<uint8_t>&& payload) {
            // Construct our NetworkSource information
            const dsl::word::NetworkSource src{peer_name, source, reliable};

            // Move the payload in as we are stealing it
            const std::vector<uint8_t> p(std::move(payload));

            /* Mutex Scope */ {
                // Lock our reaction mutex
                const std::lock_guard<std::mutex> lock(reaction_mutex);

                // Find interested reactions
                auto rs = reactions.equal_range(hash);

                // Execute on our interested reactions
                for (auto it = rs.first; it != rs.second; ++it) {
                    // Store in our thread local cache
                    dsl::store::ThreadStore<const std::vector<uint8_t>>::value     = &p;
                    dsl::store::ThreadStore<const dsl::word::NetworkSource>::value = &src;

                    powerplant.submit(it->second->get_task());
                }

                // Clear our cache
                dsl::store::ThreadStore<const std::vector<uint8_t>>::value     = nullptr;
                dsl::store::ThreadStore<const dsl::word::NetworkSource>::value = nullptr;
            }
        });

        // Set our join callback
        net.set_join_callback([this](const network::PeerInfo& peer) {
            auto l     = std::make_unique<message::NetworkJoin>();
            l->name    = peer.name;
            l->address = peer.address;
            emit(l);
        });

        // Set our leave callback
        net.set_leave_callback([this](const network::PeerInfo& peer) {
            auto l     = std::make_unique<message::NetworkLeave>();
            l->name    = peer.name;
            l->address = peer.address;
            emit(l);
        });

        // Set our event timer callback
        net.set_event_callback([this](std::chrono::steady_clock::time_point t) {
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

            // Add subscription so peers know to send us this type
            net.add_subscription(l.hash);
        });

        // Stop listening for a network type
        on<Trigger<Unbind>>().then("Network Unbind", [this](const Unbind& unbind) {
            // Lock our reaction mutex
            const std::lock_guard<std::mutex> lock(reaction_mutex);

            // Find and delete this reaction
            auto it = std::find_if(reactions.begin(), reactions.end(), [&](const auto& r) {
                return r.second->id == unbind.id;
            });
            if (it != reactions.end()) {
                reactions.erase(it);
            }

            // Rebuild subscriptions from remaining reactions
            std::set<uint64_t> subs;
            for (const auto& r : reactions) {
                subs.insert(r.first);
            }
            net.set_subscriptions(subs);
        });

        on<Trigger<NetworkEmit>>().then("Network Emit", [this](const NetworkEmit& e) {
            net.send(e.hash, e.payload.data(), e.payload.size(), e.target, e.reliable);
        });

        on<Shutdown>().then("Shutdown Network", [this] { net.shutdown(); });

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

            // Build configuration
            network::NetworkConfig net_config;
            net_config.name             = config.name.empty() ? util::get_hostname() : config.name;
            net_config.announce_address = config.announce_address;
            net_config.announce_port    = config.announce_port;
            net_config.bind_address     = config.bind_address;
            net_config.mtu              = config.mtu;

            // Collect current subscriptions
            {
                const std::lock_guard<std::mutex> lock(reaction_mutex);
                std::set<uint64_t> subs;
                for (const auto& r : reactions) {
                    subs.insert(r.first);
                }
                net.set_subscriptions(subs);
            }

            // Reset our network using this configuration
            net.reset(net_config);

            // Execution handle
            process_handle = on<Trigger<ProcessNetwork>>().then("Network processing", [this] { net.process(); });

            for (auto& fd : net.listen_fds()) {
                listen_handles.push_back(on<IO>(fd, IO::READ).then("Packet", [this] { net.process(); }));
            }
        });
    }

}  // namespace extension
}  // namespace NUClear
