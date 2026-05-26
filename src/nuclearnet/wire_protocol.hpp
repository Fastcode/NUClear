/*
 * MIT License
 *
 * Copyright (c) 2025 NUClear Contributors
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

#ifndef NUCLEAR_NETWORK_WIRE_PROTOCOL_HPP
#define NUCLEAR_NETWORK_WIRE_PROTOCOL_HPP

#include <cstddef>
#include <cstdint>

// Packing macros for wire format structs
#ifdef _MSC_VER
    #define NUCLEAR_NET_PACK(...) __pragma(pack(push, 1)) __VA_ARGS__ __pragma(pack(pop))
#else
    #define NUCLEAR_NET_PACK(...) __VA_ARGS__ __attribute__((__packed__))
#endif

namespace NUClear {
namespace network {

    /// Protocol version for NUClearNet v2
    constexpr uint8_t PROTOCOL_VERSION = 0x03;

    /// Packet type identifiers
    enum PacketType : uint8_t {
        ANNOUNCE            = 1,
        LEAVE               = 2,
        DATA                = 3,
        DATA_RETRANSMISSION = 4,
        ACK                 = 5,
        NACK                = 6,
    };

    /// Data packet flags (bit field)
    enum DataFlags : uint8_t {
        RELIABLE = 0x01,
    };

    /**
     * Header present on every NUClearNet packet.
     *
     * Wire layout (5 bytes):
     *   [0-2] 0xE2 0x98 0xA2  (UTF-8 radioactive symbol ☢)
     *   [3]   Protocol version (0x03)
     *   [4]   Packet type
     */
    NUCLEAR_NET_PACK(struct PacketHeader {
        explicit PacketHeader(PacketType t) : type(t) {}

        /// Magic bytes: radioactive symbol in UTF-8
        uint8_t header[3] = {0xE2, 0x98, 0xA2};
        /// Protocol version
        uint8_t version = PROTOCOL_VERSION;
        /// Packet type
        PacketType type;
    });

    /**
     * Announce packet — sent periodically for peer discovery.
     *
     * Wire layout (variable):
     *   [0-4]   PacketHeader (type = ANNOUNCE)
     *   [5-6]   name_length (uint16_t, network byte order)
     *   [7..7+name_length-1]  name (UTF-8, NOT null-terminated)
     *   [next 2 bytes]  num_subscriptions (uint16_t)
     *   [next num_subscriptions*8 bytes]  subscription hashes (uint64_t each)
     *
     * No port field — receiver learns the data port from the UDP source address.
     * If num_subscriptions == 0, the sender wants ALL data (no filtering).
     */
    NUCLEAR_NET_PACK(struct AnnouncePacket : PacketHeader {
        AnnouncePacket() : PacketHeader(ANNOUNCE) {}

        /// Length of the name field that follows
        uint16_t name_length{0};
        /// Variable-length data follows: name bytes, then subscription count, then subscription hashes
        /// Access via pointer arithmetic from &name_length + sizeof(uint16_t)
    });

    /**
     * Leave packet — sent on graceful shutdown.
     *
     * Wire layout (5 bytes):
     *   [0-4] PacketHeader (type = LEAVE)
     */
    NUCLEAR_NET_PACK(struct LeavePacket : PacketHeader {
        LeavePacket() : PacketHeader(LEAVE) {}
    });

    /**
     * Data packet — carries message payload (possibly one fragment of a larger message).
     *
     * Wire layout (18+ bytes):
     *   [0-4]   PacketHeader (type = DATA or DATA_RETRANSMISSION)
     *   [5-6]   packet_id (uint16_t) — unique identifier for this packet group
     *   [7-8]   packet_no (uint16_t) — fragment index within the group (0-based)
     *   [9-10]  packet_count (uint16_t) — total fragments in the group
     *   [11]    flags (uint8_t) — bit 0: reliable
     *   [12-19] hash (uint64_t) — message type identifier
     *   [20+]   payload data
     */
    NUCLEAR_NET_PACK(struct DataPacket : PacketHeader {
        DataPacket() : PacketHeader(DATA) {}

        /// Unique identifier for this packet group
        uint16_t packet_id{0};
        /// Fragment index within the group (0-based)
        uint16_t packet_no{0};
        /// Total number of fragments in the group
        uint16_t packet_count{1};
        /// Flags (bit 0 = reliable)
        uint8_t flags{0};
        /// 64-bit hash identifying the message type
        uint64_t hash{0};
        /// Payload data starts here (access via &data)
        char data{0};
    });

    /**
     * ACK packet — acknowledges receipt of data fragments.
     *
     * Wire layout (11+ bytes):
     *   [0-4]   PacketHeader (type = ACK)
     *   [5-6]   packet_id (uint16_t)
     *   [7-8]   packet_count (uint16_t)
     *   [9+]    bitset of received packets (1 bit per fragment, LSB first)
     */
    NUCLEAR_NET_PACK(struct ACKPacket : PacketHeader {
        ACKPacket() : PacketHeader(ACK) {}

        /// The packet group we are acknowledging
        uint16_t packet_id{0};
        /// Total number of fragments in the group
        uint16_t packet_count{0};
        /// Bitset of received fragments (access via &packets)
        uint8_t packets{0};
    });

    /**
     * NACK packet — requests retransmission of missing fragments.
     *
     * Wire layout (9+ bytes):
     *   [0-4]   PacketHeader (type = NACK)
     *   [5-6]   packet_id (uint16_t)
     *   [7-8]   packet_count (uint16_t)
     *   [9+]    bitset of MISSING packets (1 bit per fragment, LSB first)
     */
    NUCLEAR_NET_PACK(struct NACKPacket : PacketHeader {
        NACKPacket() : PacketHeader(NACK) {}

        /// The packet group we are requesting retransmission for
        uint16_t packet_id{0};
        /// Total number of fragments in the group
        uint16_t packet_count{0};
        /// Bitset of missing fragments (access via &packets)
        uint8_t packets{0};
    });

    /**
     * Validate that a buffer contains a valid NUClearNet packet header.
     *
     * @param data    Pointer to the received data
     * @param length  Length of the received data in bytes
     *
     * @return true if the header is valid (correct magic, correct version, valid type)
     */
    inline bool validate_header(const void* data, std::size_t length) {
        if (length < sizeof(PacketHeader)) {
            return false;
        }
        const auto* header = static_cast<const PacketHeader*>(data);
        return header->header[0] == 0xE2 && header->header[1] == 0x98 && header->header[2] == 0xA2
               && header->version == PROTOCOL_VERSION && header->type >= ANNOUNCE && header->type <= NACK;
    }

}  // namespace network
}  // namespace NUClear

#endif  // NUCLEAR_NETWORK_WIRE_PROTOCOL_HPP
