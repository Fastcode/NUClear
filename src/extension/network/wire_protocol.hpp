/*
 * MIT License
 *
 * Copyright (c) 2017 NUClear Contributors
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

#ifndef NUCLEAR_EXTENSION_NETWORK_WIRE_PROTOCOL_HPP
#define NUCLEAR_EXTENSION_NETWORK_WIRE_PROTOCOL_HPP

#include <cstdint>

// These macros are used to pack the structs so that they are sent over the network in the correct format
#ifdef _MSC_VER
    // NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
    #define PACK(...) __pragma(pack(push, 1)) __VA_ARGS__ __pragma(pack(pop))
#else
    // NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
    #define PACK(...) __VA_ARGS__ __attribute__((__packed__))
#endif

namespace NUClear {
namespace extension {
    namespace network {

        /**
         * A number that is used to represent the type of packet that is being sent/received
         */
        enum Type : uint8_t { ANNOUNCE = 1, LEAVE = 2, DATA = 3, DATA_RETRANSMISSION = 4, ACK = 5, NACK = 6 };

        /**
         * The header that is sent with every packet.
         */
        PACK(struct PacketHeader {
            explicit PacketHeader(const Type& t) : type(t) {}

            /// Radioactive symbol in UTF8
            // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
            uint8_t header[3] = {0xE2, 0x98, 0xA2};
            /// The NUClear networking version
            uint8_t version = 0x02;
            /// The type of packet
            Type type;
        });

        PACK(struct AnnouncePacket
             : PacketHeader {
                 AnnouncePacket() : PacketHeader(ANNOUNCE) {}

                 // A null terminated string name for this node (&name)
                 char name{0};
             });

        PACK(struct LeavePacket : PacketHeader{LeavePacket(): PacketHeader(LEAVE){}});

        PACK(struct DataPacket
             : PacketHeader {
                 DataPacket() : PacketHeader(DATA) {}

                 // A semi-unique identifier for this packet group
                 uint16_t packet_id{0};
                 // What packet number this is within the group
                 uint16_t packet_no{0};
                 // How many packets there are in the group
                 uint16_t packet_count{1};
                 // If this packet is reliable and should be acked
                 bool reliable{false};
                 // The 64 bit hash to identify the data type
                 uint64_t hash{0};
                 // The data (access using &data)
                 char data{0};
             });

        PACK(struct ACKPacket
             : PacketHeader {
                 ACKPacket() : PacketHeader(ACK) {}

                 /// The packet group identifier we are acknowledging
                 uint16_t packet_id{0};
                 /// The index of the packet we are acknowledging
                 uint16_t packet_no{0};
                 /// How many packets there are in the group
                 uint16_t packet_count{1};
                 /// A bitset of which packets we have received (access using &packets)
                 uint8_t packets{0};
             });

        PACK(struct NACKPacket
             : PacketHeader {
                 NACKPacket() : PacketHeader(NACK) {}

                 /// The packet group identifier we are acknowledging
                 uint16_t packet_id{0};
                 /// How many packets there are in the group
                 uint16_t packet_count{1};
                 /// A bitset of which packets we have received (access using &packets)
                 uint8_t packets{0};
             });

    }  // namespace network
}  // namespace extension
}  // namespace NUClear

#endif  // NUCLEAR_EXTENSION_NETWORK_WIRE_PROTOCOL_HPP
