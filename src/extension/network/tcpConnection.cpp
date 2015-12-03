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
#include "nuclear_bits/extension/network/WireProtocol.hpp"

#include <algorithm>
#include <cerrno>

namespace NUClear {
    namespace extension {

        void NetworkController::tcpConnection(const TCP::Connection& connection) {

            // Allocate data for our header
            std::vector<char> data(sizeof(network::PacketHeader));

            // Read our header
            ::recv(connection.fd, data.data(), data.size(), MSG_WAITALL);
            const network::PacketHeader& header = *reinterpret_cast<network::PacketHeader*>(data.data());

            // Add enough space for our remaining packet
            data.resize(data.size() + header.length);

            // Read our remaining packet
            ::recv(connection.fd, data.data() + sizeof(network::PacketHeader), header.length, MSG_WAITALL);
            const network::AnnouncePacket& announce = *reinterpret_cast<network::AnnouncePacket*>(data.data());

            // Lock our mutex to make sure we only add one at a time
            std::lock_guard<std::mutex> lock(targetMutex);

            // See if we can find our network target for this element
            auto udpT = udpTarget.find(std::make_pair(connection.remote.address, announce.udpPort));

            // If it does not already exist, insert it
            if(udpT == udpTarget.end()) {

                auto it = targets.emplace(targets.end(), std::string(&announce.name), connection.remote.address, announce.tcpPort, announce.udpPort, connection.fd);
                nameTarget.insert(std::make_pair(std::string(&announce.name), it));
                udpTarget.insert(std::make_pair(std::make_pair(connection.remote.address, announce.udpPort), it));
                tcpTarget.insert(std::make_pair(it->tcpFD, it));

                // Bind our new reaction
                it->handle = on<IO, Sync<NetworkController>>(it->tcpFD, IO::READ | IO::FAIL | IO::CLOSE).then("Network TCP Handler", [this] (const IO::Event& e) {
                    tcpHandler(e);
                });

                // emit a message that says who connected
                auto j = std::make_unique<message::NetworkJoin>();
                j->name = &announce.name;
                j->address = connection.remote.address;
                j->tcpPort = announce.tcpPort;
                j->udpPort = announce.udpPort;
                emit(j);
            }
            else {
                // Close the connection that was made, we made one to them faster
                // And as it was never bound to a callback, it won't show up as an event
                ::close(connection.fd);
            }
        }
    }
}
