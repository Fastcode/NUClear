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

#ifndef NUCLEAR_EXTENSION_NETWORK_WIREPROTOCOL_H
#define NUCLEAR_EXTENSION_NETWORK_WIREPROTOCOL_H

#include "nuclear"

namespace NUClear {
    namespace extension {
        namespace network {
            
#pragma pack(push, 1)
            enum Type : uint8_t {
                ANNOUNCE = 1,
                DATA = 2
            };
            struct PacketHeader {
                uint8_t header[3] = { 0xE2, 0x98, 0xA2 };  // Radioactive symbol in UTF8
                uint8_t version   = 0x01;                  // The NUClear networking version
                Type type;                                 // The type of packet
                uint32_t length;                           // The length of the remainder of the packet
            };
            
            struct AnnouncePacket : public PacketHeader {
                uint16_t tcpPort;               // The TCP port it is listening on
                uint16_t udpPort;               // The UDP port it is listening on
                char name;                      // A null terminated string name for this node (&name)
            };
            
            struct DataPacket : public PacketHeader  {
                uint16_t packetId;              // A semiunique identifier for this packet group
                uint16_t packetNo;              // What packet number this is
                uint16_t packetCount;           // How many packets there are
                bool multicast;                 // If this packet is targeted
                std::array<uint64_t, 2> hash;   // The 128 bit hash to identify the data type
                char data;                      // The data (&data)
            };
#pragma pack(pop)
        }
    }
}

#endif
