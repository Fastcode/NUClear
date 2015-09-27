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
        
        using dsl::word::emit::NetworkEmit;
        
        void NetworkController::udpSend(const NetworkEmit& emit) {
            
            // TODO in the future upgrade this to using udp iovec
            
            // Get an ID for this group of packets
            uint16_t packetID = ++packetIDSource;
            
            for (uint i = 0; i < emit.data.size(); i += MAX_UDP_PAYLOAD_LENGTH) {
                
                // Our buffer (and a data packet in it)
                char buffer[MAX_UDP_PAYLOAD_LENGTH + sizeof(network::DataPacket) - 1];
                network::DataPacket* p = reinterpret_cast<network::DataPacket*>(buffer);
                *p = network::DataPacket();
                
                // Calculate how much data is in this packet
                uint payloadLength = (i + MAX_UDP_PAYLOAD_LENGTH) < emit.data.size() ? MAX_UDP_PAYLOAD_LENGTH : emit.data.size() % MAX_UDP_PAYLOAD_LENGTH;
                
                p->type         = network::DATA;
                p->length       = payloadLength + sizeof(network::DataPacket) - sizeof(network::PacketHeader) - 1;
                p->packetId     = packetID;
                p->packetNo     = uint16_t(i / MAX_UDP_PAYLOAD_LENGTH);
                p->packetCount  = uint16_t((emit.data.size() / MAX_UDP_PAYLOAD_LENGTH) + 1);
                p->multicast    = emit.target.empty();
                p->hash         = emit.hash;
                std::memcpy(&p->data, emit.data.data() + i, payloadLength);
                
                // Send multicast
                if (emit.target.empty()) {
                    
                    // Multicast address
                    sockaddr_in multicastTarget;
                    std::memset(&multicastTarget, 0, sizeof(sockaddr_in));
                    multicastTarget.sin_family = AF_INET;
                    multicastTarget.sin_addr.s_addr = inet_addr(multicastGroup.c_str());
                    multicastTarget.sin_port = htons(multicastPort);
                    
                    // Send the packet
                    ::sendto(udpServerFD, buffer, p->length + sizeof(network::PacketHeader), 0, reinterpret_cast<sockaddr*>(&multicastTarget), sizeof(sockaddr));
                }
                // Send unicast
                else {
                    auto sendTo = nameTarget.equal_range(emit.target);
                    
                    for(auto it = sendTo.first; it != sendTo.second; ++it) {
                        
                        // Unicast address
                        sockaddr_in target;
                        std::memset(&target, 0, sizeof(sockaddr_in));
                        target.sin_family = AF_INET;
                        target.sin_addr.s_addr = htonl(it->second->address);
                        target.sin_port = htons(it->second->udpPort);
                        
                        // Send the packet
                        ::sendto(udpServerFD, buffer, p->length + sizeof(network::PacketHeader), 0, reinterpret_cast<sockaddr*>(&target), sizeof(sockaddr));
                    }
                }
            }
        }
    }
}
