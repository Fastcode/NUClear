/*
 * Copyright (C) 2013 Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
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
        , writeMutex()
        , udpHandle()
        , tcpHandle()
        , multicastHandle()
        , multicastEmitHandle()
        , networkEmitHandle()
        , name("")
        , multicastGroup("")
        , multicastPort(0)
        , udpPort(0)
        , tcpPort(0)
        , udpServerFD(0)
        , tcpServerFD(0)
        , packetIDSource(1)
        , reactionMutex()
        , reactions()
        , targets()
        , nameTarget()
        , udpTarget()
        , tcpTarget() {

            // Turn off sigpipe...
			#ifndef _WIN32
				::signal(SIGPIPE, SIG_IGN);
			#endif

            on<Trigger<dsl::word::NetworkListen>, Sync<NetworkController>>().then([this] (const dsl::word::NetworkListen& l) {
                // Lock our reaction mutex
                std::lock_guard<std::mutex> lock(reactionMutex);

                // Insert our new reaction
                reactions.insert(std::make_pair(l.hash, l.reaction));
            });

            on<Trigger<dsl::operation::Unbind<dsl::word::NetworkListen>>, Sync<NetworkController>>().then([this] (const dsl::operation::Unbind<dsl::word::NetworkListen>& unbind) {

                // Lock our reaction mutex
                std::lock_guard<std::mutex> lock(reactionMutex);

                // Find and delete this reaction
                for (auto it = reactions.begin(); it != reactions.end(); ++it) {
                    if (it->second->reactionId == unbind.reactionId) {
                        reactions.erase(it);
                        break;
                    }
                }
            });

            on<Trigger<message::NetworkConfiguration>, Sync<NetworkController>>().then([this] (const message::NetworkConfiguration& config) {

                // Unbind our incoming handles if they exist
                if (udpHandle) {
                    udpHandle.unbind();
                }
                if (tcpHandle) {
                    tcpHandle.unbind();
                }
                if (multicastHandle) {
                    multicastHandle.unbind();
                }
                if (multicastEmitHandle) {
                    multicastEmitHandle.unbind();
                }
                if (networkEmitHandle) {
                    networkEmitHandle.unbind();
                }

                // Unbind any TCP connections we may have and clear our maps
                for (auto& target : targets) {
                    target.handle.unbind();
                    close(target.tcpFD);
                }

                // Clear all our maps
                targets.clear();
                nameTarget.clear();
                udpTarget.clear();
                tcpTarget.clear();

                // Store our new configuration
                if(config.name.empty()) {
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
                multicastGroup = config.multicastGroup;
                multicastPort = config.multicastPort;

                // Add our new reactions
                std::tie(udpHandle, udpPort, udpServerFD) = on<UDP, Sync<NetworkController>>().then([this] (const UDP::Packet& packet) {
                    udpHandler(packet);
                });

                std::tie(tcpHandle, tcpPort, tcpServerFD) = on<TCP, Sync<NetworkController>>().then([this] (const TCP::Connection& connection) {
                    tcpConnection(connection);
                });

                std::tie(multicastHandle, std::ignore, std::ignore) = on<UDP::Multicast, Sync<NetworkController>>(multicastGroup, multicastPort).then([this] (const UDP::Packet& packet) {
                    udpHandler(packet);
                });

                multicastEmitHandle = on<Every<1, std::chrono::seconds>, Single, Sync<NetworkController>>().then([this] {
                    announce();
                });

                networkEmitHandle = on<Trigger<dsl::word::emit::NetworkEmit>, Sync<NetworkController>>().then([this] (const dsl::word::emit::NetworkEmit& emit) {
                    // See if this message should be sent reliably
                    if(emit.reliable) {
                        tcpSend(emit);
                    }
                    else {
                        udpSend(emit);
                    }
                });
            });
        }
    }
}
