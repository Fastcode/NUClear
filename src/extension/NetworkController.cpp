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

#include "nuclear_bits/extension/NetworkController.hpp"

#ifdef _WIN32
#include "nuclear_bits/util/platform.hpp"
#else
#include <sys/utsname.h>
#endif

#include <algorithm>
#include <cerrno>
#include <csignal>

namespace NUClear {
namespace extension {

    NetworkController::NetworkController(std::unique_ptr<NUClear::Environment> environment)
        : Reactor(std::move(environment))
        , write_mutex()
        , udp_handle()
        , tcp_handle()
        , multicast_handle()
        , multicast_emit_handle()
        , network_emit_handle()
        , name("")
        , multicast_group("")
        , multicast_port(0)
        , udp_port(0)
        , tcp_port(0)
        , udp_server_fd(0)
        , tcp_server_fd(0)
        , packet_id_source(1)
        , reaction_mutex()
        , reactions()
        , targets()
        , name_target()
        , udp_target()
        , tcp_target() {

// Turn off sigpipe...
#ifndef _WIN32
        ::signal(SIGPIPE, SIG_IGN);
#endif

        on<Trigger<dsl::word::NetworkListen>, Sync<NetworkController>>().then(
            [this](const dsl::word::NetworkListen& l) {
                // Lock our reaction mutex
                std::lock_guard<std::mutex> lock(reaction_mutex);

                // Insert our new reaction
                reactions.insert(std::make_pair(l.hash, l.reaction));
            });

        on<Trigger<dsl::operation::Unbind<dsl::word::NetworkListen>>, Sync<NetworkController>>().then(
            [this](const dsl::operation::Unbind<dsl::word::NetworkListen>& unbind) {

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

        on<Trigger<message::NetworkConfiguration>, Sync<NetworkController>>().then(
            [this](const message::NetworkConfiguration& config) {

                // Unbind our incoming handles if they exist
                if (udp_handle) {
                    udp_handle.unbind();
                }
                if (tcp_handle) {
                    tcp_handle.unbind();
                }
                if (multicast_handle) {
                    multicast_handle.unbind();
                }
                if (multicast_emit_handle) {
                    multicast_emit_handle.unbind();
                }
                if (network_emit_handle) {
                    network_emit_handle.unbind();
                }

                // Unbind any TCP connections we may have and clear our maps
                for (auto& target : targets) {
                    target.handle.unbind();
                    close(target.tcp_fd);
                }

                // Clear all our maps
                targets.clear();
                name_target.clear();
                udp_target.clear();
                tcp_target.clear();

                // Store our new configuration
                if (config.name.empty()) {
// If our config name is empty, use our system name
#ifdef _WIN32
                    char n[MAX_COMPUTERNAME_LENGTH + 1];
                    DWORD size = sizeof(n);
                    GetComputerName(n, &size);
                    name = std::string(n, size);
#else
                    utsname u;
                    uname(&u);
                    name = u.nodename;
#endif
                }
                else {
                    name = config.name;
                }
                multicast_group = config.multicast_group;
                multicast_port  = config.multicast_port;

                // Add our new reactions
                std::tie(udp_handle, udp_port, udp_server_fd) =
                    on<UDP, Sync<NetworkController>>().then([this](const UDP::Packet& packet) { udp_handler(packet); });

                std::tie(tcp_handle, tcp_port, tcp_server_fd) = on<TCP, Sync<NetworkController>>().then(
                    [this](const TCP::Connection& connection) { tcp_connection(connection); });

                std::tie(multicast_handle, std::ignore, std::ignore) =
                    on<UDP::Multicast, Sync<NetworkController>>(multicast_group, multicast_port)
                        .then([this](const UDP::Packet& packet) { udp_handler(packet); });

                multicast_emit_handle =
                    on<Every<1, std::chrono::seconds>, Single, Sync<NetworkController>>().then([this] { announce(); });

                network_emit_handle = on<Trigger<dsl::word::emit::NetworkEmit>, Sync<NetworkController>>().then(
                    [this](const dsl::word::emit::NetworkEmit& emit) {
                        // See if this message should be sent reliably
                        if (emit.reliable) {
                            tcp_send(emit);
                        }
                        else {
                            udp_send(emit);
                        }
                    });
            });
    }
}
}
