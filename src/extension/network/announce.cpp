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

        void NetworkController::announce() {

            // Make a data vector of the correct size and default values
            std::vector<char> packet(sizeof(network::AnnouncePacket) + name.size());
            network::AnnouncePacket* p = reinterpret_cast<network::AnnouncePacket*>(packet.data());
            *p = network::AnnouncePacket();

            // Make an announce packet
            p->type = network::ANNOUNCE;
            p->tcpPort = tcpPort;
            p->udpPort = udpPort;

            // Length is the size without the header
            p->length = uint32_t(sizeof(network::AnnouncePacket) + name.size() - sizeof(network::PacketHeader));

            // Copy our name over
            std::memcpy(&p->name, name.c_str(), name.size() + 1);

            // Send a multicast packet announcing ourselves from our UDP port
            sockaddr_in multicastTarget;
            std::memset(&multicastTarget, 0, sizeof(sockaddr_in));

            multicastTarget.sin_family = AF_INET;
            multicastTarget.sin_addr.s_addr = inet_addr(multicastGroup.c_str());
            multicastTarget.sin_port = htons(multicastPort);

            // Send the packet
            ::sendto(udpServerFD, packet.data(), packet.size(), 0, reinterpret_cast<sockaddr*>(&multicastTarget), sizeof(sockaddr));
        }
    }
}
