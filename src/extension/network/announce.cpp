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
#include "nuclear_bits/extension/network/wire_protocol.hpp"

#include <algorithm>
#include <cerrno>

namespace NUClear {
namespace extension {

    void NetworkController::announce() {

        // Make a data vector of the correct size and default values
        std::vector<char> packet(sizeof(network::AnnouncePacket) + name.size());
        network::AnnouncePacket* p = reinterpret_cast<network::AnnouncePacket*>(packet.data());
        *p                         = network::AnnouncePacket();

        // Make an announce packet
        p->type     = network::ANNOUNCE;
        p->tcp_port = tcp_port;
        p->udp_port = udp_port;

        // Length is the size without the header
        p->length = uint32_t(sizeof(network::AnnouncePacket) + name.size() - sizeof(network::PacketHeader));

        // Copy our name over
        std::memcpy(&p->name, name.c_str(), name.size() + 1);

        // Send a multicast packet announcing ourselves from our UDP port
        sockaddr_in multicast_target;
        std::memset(&multicast_target, 0, sizeof(sockaddr_in));
        multicast_target.sin_family = AF_INET;
        inet_pton(AF_INET, multicast_group.c_str(), &multicast_target.sin_addr);
        multicast_target.sin_port = htons(multicast_port);

        // Send the packet
        ::sendto(udp_server_fd,
                 packet.data(),
                 packet.size(),
                 0,
                 reinterpret_cast<sockaddr*>(&multicast_target),
                 sizeof(sockaddr));
    }
}
}
