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

    void NetworkController::udp_handler(const UDP::Packet& packet) {

        // TODO work out if this is us sending multicast to ourselves and if so return
        // Indicators, udp_port = our_udp_port
        // packet.remote.address == ours (? but how do we know our address?)

        // Work out what type of packet this is
        const network::PacketHeader& header = *reinterpret_cast<const network::PacketHeader*>(packet.payload.data());

        if (header.type == network::ANNOUNCE) {

            const network::AnnouncePacket& announce =
                *reinterpret_cast<const network::AnnouncePacket*>(packet.payload.data());

            std::string new_name = &announce.name;
            int new_tcp_port     = announce.tcp_port;
            int new_udp_port     = announce.udp_port;

            // Make sure this packet isn't suspect
            if (packet.remote.port == new_udp_port) {

                // Make sure this is not us
                if (!(name == new_name && tcp_port == new_tcp_port && udp_port == new_udp_port)) {

                    // Check we do not already have this client connected
                    if (udp_target.find(std::make_pair(packet.remote.address, new_udp_port)) == udp_target.end()) {

                        sockaddr_in local;
                        std::memset(&local, 0, sizeof(sockaddr_in));
                        local.sin_family      = AF_INET;
                        local.sin_port        = htons(tcp_port);
                        local.sin_addr.s_addr = htonl(INADDR_ANY);

                        sockaddr_in remote;
                        std::memset(&remote, 0, sizeof(sockaddr_in));
                        remote.sin_family      = AF_INET;
                        remote.sin_port        = htons(new_tcp_port);
                        remote.sin_addr.s_addr = htonl(packet.remote.address);

                        // Open a TCP connection
                        util::FileDescriptor tcp_fd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

                        ::connect(tcp_fd, reinterpret_cast<sockaddr*>(&remote), sizeof(sockaddr));
                        // TODO this might get closed or refused or something else

                        // Make a data vector of the correct size and default values
                        std::vector<char> announce_packet(sizeof(network::AnnouncePacket) + name.size());
                        network::AnnouncePacket* p = reinterpret_cast<network::AnnouncePacket*>(announce_packet.data());
                        *p                         = network::AnnouncePacket();

                        // Make an announce packet
                        p->type     = network::ANNOUNCE;
                        p->tcp_port = tcp_port;
                        p->udp_port = udp_port;

                        // Length is the size without the header
                        p->length =
                            uint32_t(sizeof(network::AnnouncePacket) + name.size() - sizeof(network::PacketHeader));

                        // Copy our name over
                        std::memcpy(&p->name, name.c_str(), name.size() + 1);

                        // Copy our packet data and name over
                        ::send(tcp_fd, announce_packet.data(), announce_packet.size(), 0);

                        // Insert our new element
                        auto it = targets.emplace(targets.end(),
                                                  new_name,
                                                  packet.remote.address,
                                                  new_tcp_port,
                                                  new_udp_port,
                                                  tcp_fd.release());
                        name_target.insert(std::make_pair(new_name, it));
                        udp_target.insert(std::make_pair(std::make_pair(packet.remote.address, new_udp_port), it));
                        tcp_target.insert(std::make_pair(it->tcp_fd, it));

                        // Start our connected handle
                        it->handle = on<IO, Sync<NetworkController>>(it->tcp_fd, IO::READ | IO::ERROR | IO::CLOSE)
                                         .then("Network TCP Handler", [this](const IO::Event& e) { tcp_handler(e); });

                        // emit a message that says who connected
                        auto j      = std::make_unique<message::NetworkJoin>();
                        j->name     = &announce.name;
                        j->address  = packet.remote.address;
                        j->tcp_port = announce.tcp_port;
                        j->udp_port = announce.udp_port;
                        emit(j);
                    }
                }
            }
        }
        else if (header.type == network::DATA) {

            // Work out who our remote is
            auto remote = udp_target.find(std::make_pair(packet.remote.address, packet.remote.port));

            // Check if we know who this is and if we don't know them, ignore
            if (remote != udp_target.end()) {

                const network::DataPacket& p = *reinterpret_cast<const network::DataPacket*>(packet.payload.data());

                // If this is a solo packet (in a single chunk)
                if (p.packet_no == 0 && p.packet_count == 1) {

                    // Copy our data into a vector
                    std::vector<char> payload(
                        &p.data, &p.data + p.length - sizeof(network::DataPacket) + sizeof(network::PacketHeader) + 1);

                    // Construct our NetworkSource information
                    dsl::word::NetworkSource src;
                    src.name      = remote->second->name;
                    src.address   = remote->second->address;
                    src.port      = remote->second->udp_port;
                    src.reliable  = true;
                    src.multicast = p.multicast;

                    // Store in our thread local cache
                    dsl::store::ThreadStore<std::vector<char>>::value        = &payload;
                    dsl::store::ThreadStore<dsl::word::NetworkSource>::value = &src;

                    /* Mutex Scope */ {
                        // Lock our reaction mutex
                        std::lock_guard<std::mutex> lock(reaction_mutex);

                        // Find interested reactions
                        auto rs = reactions.equal_range(p.hash);

                        // Execute on our interested reactions
                        for (auto it = rs.first; it != rs.second; ++it) {
                            auto task = it->second->get_task();
                            if (task) {
                                powerplant.submit(std::move(task));
                            }
                        }
                    }

                    // Clear our cache
                    dsl::store::ThreadStore<std::vector<char>>::value        = nullptr;
                    dsl::store::ThreadStore<dsl::word::NetworkSource>::value = nullptr;
                }
                else {
                    std::lock_guard<std::mutex> lock(remote->second->buffer_mutex);

                    // Get our buffer
                    auto& buffer = remote->second->buffer;

                    // Check if we have too many packets
                    if (buffer.size() > MAX_NUM_UDP_ASSEMBLY) {
                        // Remove the oldest assembly target
                        auto oldest = buffer.begin();

                        for (auto it = buffer.begin(); it != buffer.end(); ++it) {
                            if (it->second.first < oldest->second.first) {
                                oldest = it;
                            }
                        }

                        buffer.erase(oldest);
                    }

                    auto& set = buffer[p.packet_id];
                    set.first = clock::now();

                    // Add our packet
                    set.second.push_back(packet.payload);

                    // If we are finished assemble and emit
                    if (set.second.size() == p.packet_count) {

                        // Our final payload
                        std::vector<char> payload;

                        // Sort the list
                        std::sort(set.second.begin(),
                                  set.second.end(),
                                  [](const std::vector<char>& a, const std::vector<char>& b) {
                                      const network::DataPacket& p_a =
                                          *reinterpret_cast<const network::DataPacket*>(a.data());
                                      const network::DataPacket& p_b =
                                          *reinterpret_cast<const network::DataPacket*>(b.data());
                                      return p_a.packet_no < p_b.packet_no;
                                  });

                        // Copy the data across
                        for (auto& v : set.second) {
                            payload.insert(payload.end(), v.begin() + sizeof(network::DataPacket) - 1, v.end());
                        }

                        // Construct our NetworkSource information
                        dsl::word::NetworkSource src;
                        src.name      = remote->second->name;
                        src.address   = remote->second->address;
                        src.port      = remote->second->udp_port;
                        src.reliable  = true;
                        src.multicast = p.multicast;

                        // Store in our thread local cache
                        dsl::store::ThreadStore<std::vector<char>>::value        = &payload;
                        dsl::store::ThreadStore<dsl::word::NetworkSource>::value = &src;

                        /* Mutex Scope */ {
                            // Lock our reaction mutex
                            std::lock_guard<std::mutex> lock(reaction_mutex);

                            // Find interested reactions
                            auto rs = reactions.equal_range(p.hash);

                            // Execute on our interested reactions
                            for (auto it = rs.first; it != rs.second; ++it) {
                                auto task = it->second->get_task();
                                if (task) {
                                    powerplant.submit(std::move(task));
                                }
                            }
                        }

                        // Clear our cache
                        dsl::store::ThreadStore<std::vector<char>>::value        = nullptr;
                        dsl::store::ThreadStore<dsl::word::NetworkSource>::value = nullptr;


                        // Erase this from the list
                        buffer.erase(buffer.find(p.packet_id));
                    }
                }
            }
        }
    }
}
}
