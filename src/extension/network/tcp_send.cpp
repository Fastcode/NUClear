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
    using dsl::word::emit::NetworkEmit;

    void NetworkController::tcp_send(const NetworkEmit& emit) {

        // Construct a packet to send
        network::DataPacket p;
        p.type      = network::DATA;
        p.length    = uint32_t((sizeof(network::DataPacket) - 1) - sizeof(network::PacketHeader) + emit.payload.size());
        p.packet_id = ++packet_id_source;
        p.packet_no = 0;     // TCP packets always come in one packet
        p.packet_count = 1;  // TCP packets always come in one packet
        p.multicast    = emit.target.empty();
        p.hash         = emit.hash;

        // Stores who we should send to
        decltype(name_target.equal_range(emit.target)) send_to;

        // Send to everyone
        if (emit.target.empty()) {
            send_to = std::make_pair(name_target.begin(), name_target.end());
        }
        // Send to one name
        else {
            send_to = name_target.equal_range(emit.target);
        }

        for (auto it = send_to.first; it != send_to.second; ++it) {
            // Write the header (except the blank "char" value) and then the packet
            ::send(it->second->tcp_fd, reinterpret_cast<const char*>(&p), sizeof(network::DataPacket) - 1, 0);
            ::send(it->second->tcp_fd, emit.payload.data(), emit.payload.size(), 0);
            // TODO do this as a single send
        }
    }
}
}
