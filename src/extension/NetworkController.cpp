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

#include "../LogLevel.hpp"
#include "../Reactor.hpp"
#include "../dsl/operation/Unbind.hpp"
#include "../dsl/store/ThreadStore.hpp"
#include "../dsl/word/Network.hpp"
#include "../dsl/word/emit/Network.hpp"
#include "../message/NetworkConfiguration.hpp"
#include "../message/NetworkEvent.hpp"
#include "../nuclearnet/Discovery.hpp"
#include "../nuclearnet/Log.hpp"
#include "../nuclearnet/NUClearNet.hpp"
#include "../util/get_hostname.hpp"

namespace NUClear {
namespace extension {

    using NetworkListen        = dsl::word::NetworkListen;
    using NetworkEmit          = dsl::word::emit::NetworkEmit;
    using NetworkConfiguration = message::NetworkConfiguration;
    using Unbind               = dsl::operation::Unbind<NetworkListen>;
    struct ProcessNetwork {};

namespace {

    /// Convert a NUClear log level into the equivalent level for the NUClearNet library
    network::LogLevel to_network_level(const LogLevel& level) {
        switch (level) {
            case LogLevel::TRACE: return network::LogLevel::Trace;
            case LogLevel::DEBUG: return network::LogLevel::Debug;
            case LogLevel::INFO: return network::LogLevel::Info;
            case LogLevel::WARN: return network::LogLevel::Warn;
            case LogLevel::ERROR:
            case LogLevel::FATAL: return network::LogLevel::Error;
            default: return network::LogLevel::Off;
        }
    }

    /// Convert a NUClearNet library log level into the equivalent NUClear log level
    LogLevel from_network_level(const network::LogLevel& level) {
        switch (level) {
            case network::LogLevel::Trace: return LogLevel::TRACE;
            case network::LogLevel::Debug: return LogLevel::DEBUG;
            case network::LogLevel::Info: return LogLevel::INFO;
            case network::LogLevel::Warn: return LogLevel::WARN;
            case network::LogLevel::Error: return LogLevel::ERROR;
            default: return LogLevel::UNKNOWN;
        }
    }

}  // namespace

    NetworkController::NetworkController(std::unique_ptr<NUClear::Environment> environment)
        : Reactor(std::move(environment)) {

        // Send the NUClearNet library's logs through the NUClear logging system rather than to stderr.
        // Every path that logs (reset, send, process) is only ever called from within one of our reactions so we
        // will always have reactor context when this fires.
        network::NUClearNet::set_log_handler(
            [this](const network::LogLevel& level, const char* component, const std::string& message) {
                const std::string text = std::string("NUClearNet:") + component + " " + message;

                // Dispatch on the level explicitly. Reactor::log(level, args...) resolves to the compile time
                // overload with its default level, which would log everything at DEBUG with the level name
                // stringified into the message.
                switch (from_network_level(level)) {
                    case LogLevel::TRACE: log<LogLevel::TRACE>(text); break;
                    case LogLevel::DEBUG: log<LogLevel::DEBUG>(text); break;
                    case LogLevel::INFO: log<LogLevel::INFO>(text); break;
                    case LogLevel::WARN: log<LogLevel::WARN>(text); break;
                    case LogLevel::ERROR: log<LogLevel::ERROR>(text); break;
                    case LogLevel::FATAL: log<LogLevel::FATAL>(text); break;
                    default: break;
                }
            });

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

        // When the sockets are replaced after a rebind, update the IO event registrations
        net.set_socket_change_callback([this] {
            for (auto& h : listen_handles) {
                h.unbind();
            }
            listen_handles.clear();
            for (auto& fd : net.listen_fds()) {
                listen_handles.push_back(on<IO>(fd, IO::READ).then("Packet", [this] { net.process(); }));
            }
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
            // Pass the configured log level through to both this reactor and the NUClearNet library.
            // Both are needed, the library level decides what it hands us and our level decides what gets emitted.
            // UNKNOWN means the level wasn't configured, so leave our level alone and keep the library quiet.
            if (config.log_level != LogLevel::UNKNOWN) {
                this->log_level = config.log_level;
            }
            net.set_log_level(to_network_level(config.log_level));

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

    NetworkController::~NetworkController() {
        // Put the logs back on stderr so the library can't call back into us once we are gone
        network::NUClearNet::set_log_handler(nullptr);
    }

}  // namespace extension
}  // namespace NUClear
