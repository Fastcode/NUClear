/*
 * Copyright (C) 2013      Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
 *               2014-2017 Trent Houliston <trent@houliston.me>
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

namespace NUClear {
namespace extension {
    namespace network {

#pragma pack(push, 1)
        enum Type : uint8_t { ANNOUNCE = 1, LEAVE = 2, DATA = 3, DATA_RETRANSMISSION = 4, ACK = 5, NACK = 6 };
        struct PacketHeader {
            PacketHeader(const Type& t) : type(t) {}

            // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
            uint8_t header[3] = {0xE2, 0x98, 0xA2};  // Radioactive symbol in UTF8
            uint8_t version   = 0x02;                // The NUClear networking version
            Type type;                               // The type of packet
        };

        struct AnnouncePacket : public PacketHeader {
            AnnouncePacket() : PacketHeader(ANNOUNCE) {}

            char name{0};  // A null terminated string name for this node (&name)
        };

        struct LeavePacket : public PacketHeader {
            LeavePacket() : PacketHeader(LEAVE) {}
        };

        struct DataPacket : public PacketHeader {
            DataPacket() : PacketHeader(DATA) {}

            uint16_t packet_id{0};     // A semi-unique identifier for this packet group
            uint16_t packet_no{0};     // What packet number this is within the group
            uint16_t packet_count{1};  // How many packets there are in the group
            bool reliable{false};      // If this packet is reliable and should be acked
            uint64_t hash{0};          // The 64 bit hash to identify the data type
            char data{0};              // The data (&data)
        };

        struct ACKPacket : public PacketHeader {
            ACKPacket() : PacketHeader(ACK) {}

            uint16_t packet_id{0};     // The packet group identifier we are acknowledging
            uint16_t packet_no{0};     // The index of the packet we are acknowledging
            uint16_t packet_count{1};  // How many packets there are in the group
            uint8_t packets{0};        // A bitset of which packets we have received (&packets)
        };

        struct NACKPacket : public PacketHeader {

            NACKPacket() : PacketHeader(NACK) {}

            uint16_t packet_id{0};     // The packet group identifier we are acknowledging
            uint16_t packet_count{1};  // How many packets there are in the group
            uint8_t packets{0};        // A bitset of which packets we have received (&packets)
        };

#pragma pack(pop)

    }  // namespace network
}  // namespace extension
}  // namespace NUClear

#endif  // NUCLEAR_EXTENSION_NETWORK_WIRE_PROTOCOL_HPP
