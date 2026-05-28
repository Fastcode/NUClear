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

#include "Fragmentation.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <utility>
#include <vector>

namespace NUClear {
namespace network {

    Fragmentation::Fragmentation(uint16_t packet_mtu,
                                 std::size_t max_assembly_size,
                                 std::chrono::steady_clock::duration assembly_timeout)
        : packet_mtu(packet_mtu), max_assembly_size(max_assembly_size), assembly_timeout(assembly_timeout) {}

    std::vector<Fragmentation::Fragment> Fragmentation::fragment(uint16_t packet_id,
                                                                  uint64_t hash,
                                                                  uint8_t flags,
                                                                  const std::vector<uint8_t>& payload) const {
        // Calculate how many fragments we need
        const uint16_t packet_count =
            payload.empty() ? 1 : static_cast<uint16_t>((payload.size() + packet_mtu - 1) / packet_mtu);

        std::vector<Fragment> fragments;
        fragments.reserve(packet_count);

        for (uint16_t i = 0; i < packet_count; ++i) {
            Fragment frag;
            frag.packet_id    = packet_id;
            frag.packet_no    = i;
            frag.packet_count = packet_count;
            frag.flags        = flags;
            frag.hash         = hash;

            // Calculate the data slice for this fragment
            const std::size_t offset = static_cast<std::size_t>(i) * packet_mtu;
            const std::size_t length = std::min(static_cast<std::size_t>(packet_mtu), payload.size() - offset);
            frag.data.assign(payload.data() + offset, payload.data() + offset + length);

            fragments.push_back(std::move(frag));
        }

        return fragments;
    }

    bool Fragmentation::submit_fragment(uint64_t source_key,
                                        uint16_t packet_id,
                                        uint16_t packet_no,
                                        uint16_t packet_count,
                                        uint64_t hash,
                                        uint8_t flags,
                                        const uint8_t* data,
                                        std::size_t data_length,
                                        AssembledPacket& out_packet,
                                        std::chrono::steady_clock::time_point now) {
        if (packet_count == 0 || packet_no >= packet_count) {
            return false;
        }

        // Enforce max assembly size check
        if (max_assembly_size > 0) {
            const std::size_t projected_size = static_cast<std::size_t>(packet_count) * packet_mtu;
            if (projected_size > max_assembly_size) {
                return false;
            }
        }

        const AssemblyKey key{source_key, packet_id};

        const std::lock_guard<std::mutex> lock(assembly_mutex);

        auto& assembly          = assemblies[key];
        assembly.hash           = hash;
        assembly.flags          = flags;
        assembly.packet_count   = packet_count;
        assembly.last_update    = now;

        // Store this fragment
        assembly.fragments[packet_no].assign(data, data + data_length);

        // Check if we have all fragments
        if (assembly.fragments.size() == packet_count) {
            // Assemble the complete payload
            out_packet.packet_id = packet_id;
            out_packet.hash      = hash;
            out_packet.flags     = flags;

            // Calculate total size
            std::size_t total_size = 0;
            for (const auto& fragment_entry : assembly.fragments) {
                const auto& frag_data = fragment_entry.second;
                total_size += frag_data.size();
            }
            out_packet.payload.clear();
            out_packet.payload.reserve(total_size);

            // Concatenate in order
            for (uint16_t i = 0; i < packet_count; ++i) {
                const auto& frag_data = assembly.fragments[i];
                out_packet.payload.insert(out_packet.payload.end(), frag_data.begin(), frag_data.end());
            }

            // Remove the completed assembly
            assemblies.erase(key);

            return true;
        }

        return false;
    }

    std::size_t Fragmentation::cleanup_expired(std::chrono::steady_clock::time_point now) {
        std::size_t removed = 0;

        const std::lock_guard<std::mutex> lock(assembly_mutex);
        for (auto it = assemblies.begin(); it != assemblies.end();) {
            if (now - it->second.last_update > assembly_timeout) {
                it = assemblies.erase(it);
                ++removed;
            }
            else {
                ++it;
            }
        }

        return removed;
    }

}  // namespace network
}  // namespace NUClear
