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

#include "nuclear_bits/extension/network/NUClearNetwork.hpp"

#include <sys/ioctl.h>
#include <algorithm>
#include <cerrno>

namespace NUClear {
namespace extension {
    namespace network {

        NUClearNetwork::NUClearNetwork()
            : name("")
            , udp_port(-1)
            , multicast_target()
            , unicast_fd(-1)
            , multicast_fd(-1)
            , announce_packet()
            , packet_id_source(0)
            , packet_callback()
            , join_callback()
            , leave_callback()
            , targets()
            , name_target()
            , udp_target() {}


        NUClearNetwork::~NUClearNetwork() {
            shutdown();
        }

        void NUClearNetwork::set_packet_callback(
            std::function<void(const NetworkTarget&, const std::array<uint64_t, 2>&, std::vector<char>&&)> f) {
            packet_callback = f;
        }


        void NUClearNetwork::set_join_callback(std::function<void(std::string, sockaddr)> f) {
            join_callback = f;
        }


        void NUClearNetwork::set_leave_callback(std::function<void(std::string, sockaddr)> f) {
            leave_callback = f;
        }


        std::list<NUClearNetwork::NetworkTarget>::iterator NUClearNetwork::remove_target(
            std::list<NetworkTarget>::iterator target) {

            // Get our keys for our maps
            const sockaddr_in& addr = *reinterpret_cast<const sockaddr_in*>(&target->target);
            std::string str_key     = target->name;
            auto udp_key            = std::make_pair(addr.sin_addr.s_addr, addr.sin_port);

            // Erase them
            if (udp_target.find(udp_key) != udp_target.end()) {
                udp_target.erase(udp_target.find(udp_key));
            }

            auto range = name_target.equal_range(name);
            for (auto it = range.first; it != range.second; ++it) {
                if (it->second == target) {
                    name_target.erase(it);
                    break;
                }
            }

            return targets.erase(target);
        }


        void NUClearNetwork::open_unicast() {
            unicast_fd = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            if (unicast_fd < 0) {
                throw std::system_error(network_errno, std::system_category(), "Unable to open the UDP socket");
            }

            // The address we bind to has port 0 (find a free port)
            sockaddr_in address;
            memset(&address, 0, sizeof(sockaddr_in));
            address.sin_family      = AF_INET;
            address.sin_addr.s_addr = htonl(INADDR_ANY);
            address.sin_port        = 0;

            // Bind to the address, and if we fail throw an error
            if (::bind(unicast_fd, reinterpret_cast<sockaddr*>(&address), sizeof(sockaddr))) {
                throw std::system_error(
                    network_errno, std::system_category(), "Unable to bind the UDP socket to the port");
            }

            // Get the port we ended up listening on
            socklen_t len = sizeof(sockaddr_in);
            if (::getsockname(unicast_fd, reinterpret_cast<sockaddr*>(&address), &len) == -1) {
                throw std::system_error(
                    network_errno, std::system_category(), "Unable to get the port from the UDP socket");
            }
            udp_port = ntohs(address.sin_port);
        }


        void NUClearNetwork::open_multicast() {

            // Our multicast group address
            sockaddr_in address;
            std::memset(&address, 0, sizeof(address));
            address.sin_family      = AF_INET;
            address.sin_addr.s_addr = htonl(INADDR_ANY);
            address.sin_port        = reinterpret_cast<sockaddr_in*>(&multicast_target)->sin_port;

            // Make our socket
            multicast_fd = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            if (multicast_fd < 0) {
                throw std::system_error(network_errno, std::system_category(), "Unable to open the UDP socket");
            }

            // Set that we reuse the address so more than one application can bind
            int yes = true;
            if (::setsockopt(multicast_fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char*>(&yes), sizeof(yes)) < 0) {
                throw std::system_error(network_errno, std::system_category(), "Unable to reuse address on the socket");
            }

// If SO_REUSEPORT is available set it too
#ifdef SO_REUSEPORT
            if (::setsockopt(multicast_fd, SOL_SOCKET, SO_REUSEPORT, reinterpret_cast<char*>(&yes), sizeof(yes)) < 0) {
                throw std::system_error(network_errno, std::system_category(), "Unable to reuse port on the socket");
            }
#endif

            // Bind to the address
            if (::bind(multicast_fd, reinterpret_cast<sockaddr*>(&address), sizeof(sockaddr))) {
                throw std::system_error(network_errno, std::system_category(), "Unable to bind the UDP socket");
            }

            // Our multicast join request
            ip_mreq mreq;
            memset(&mreq, 0, sizeof(mreq));
            mreq.imr_multiaddr = reinterpret_cast<sockaddr_in*>(&multicast_target)->sin_addr;

            // Join the multicast group on all the interfaces that support it
            for (auto& iface : util::network::get_interfaces()) {

                // If it is a multicast address
                if (iface.flags.multicast) {

                    // Set the interface
                    mreq.imr_interface.s_addr = htonl(iface.ip);

                    // Join our multicast group
                    if (::setsockopt(multicast_fd,
                                     IPPROTO_IP,
                                     IP_ADD_MEMBERSHIP,
                                     reinterpret_cast<char*>(&mreq),
                                     sizeof(ip_mreq))
                        < 0) {
                        throw std::system_error(network_errno,
                                                std::system_category(),
                                                "There was an error while attempting to join the multicast group");
                    }
                }
            }
        }


        void NUClearNetwork::shutdown() {

            // If we have an fd, send a shutdown message
            if (unicast_fd > 0) {
                // Make a leave packet from our announce packet
                LeavePacket packet;
                packet.type = LEAVE;

                // Send the packet
                ::sendto(unicast_fd, &packet, sizeof(packet), 0, &multicast_target, sizeof(sockaddr));
            }

            // Close our existing FDs if they exist
            if (unicast_fd > 0) {
                ::close(unicast_fd);
                unicast_fd = -1;
            }
            if (multicast_fd > 0) {
                ::close(multicast_fd);
                multicast_fd = -1;
            }
        }


        void NUClearNetwork::reset(std::string name, std::string group, in_port_t port) {

            shutdown();
            // Close our existing FDs if they exist
            if (unicast_fd > 0) {
                ::close(unicast_fd);
                unicast_fd = -1;
            }
            if (multicast_fd > 0) {
                ::close(multicast_fd);
                multicast_fd = -1;
            }

            // Store our name
            this->name = name;

            // Work out our multicast address
            sockaddr_in target;
            std::memset(&target, 0, sizeof(sockaddr_in));
            target.sin_family = AF_INET;
            inet_pton(AF_INET, group.c_str(), &target.sin_addr);
            target.sin_port  = htons(port);
            multicast_target = *reinterpret_cast<sockaddr*>(&target);

            // Build our announce packet
            announce_packet.resize(sizeof(AnnouncePacket) + name.size(), 0);
            AnnouncePacket& pkt = *reinterpret_cast<AnnouncePacket*>(announce_packet.data());
            pkt                 = AnnouncePacket();
            pkt.type            = ANNOUNCE;
            std::memcpy(&pkt.name, name.c_str(), name.size());

            // Open our unicast socket and then our multicast one
            open_unicast();
            try {
                open_multicast();
            }
            catch (std::exception& e) {
                std::cout << e.what() << std::endl;
            }
        }


        std::pair<sockaddr, std::vector<char>> NUClearNetwork::read_socket(int fd) {

            // Allocate a vector that can hold a datagram
            std::vector<char> payload(1500);
            iovec iov;
            iov.iov_base = payload.data();
            iov.iov_len  = payload.size();

            // Setup our message header to receive
            msghdr mh;
            sockaddr from;
            memset(&mh, 0, sizeof(msghdr));
            mh.msg_name    = &from;
            mh.msg_namelen = sizeof(sockaddr);
            mh.msg_iov     = &iov;
            mh.msg_iovlen  = 1;

            // Now read the data for real
            ssize_t received = recvmsg(fd, &mh, 0);
            payload.resize(received);

            return std::make_pair(from, std::move(payload));
        }


        void NUClearNetwork::process() {

            // Check if we have a packet available on the multicast socket
            int count = 0;
            ioctl(multicast_fd, FIONREAD, &count);

            if (count > 0) {
                auto packet = read_socket(multicast_fd);
                process_packet(std::move(packet.first), std::move(packet.second));
            }

            // Check if we have a packet available on the unicast socket
            count = 0;
            ioctl(unicast_fd, FIONREAD, &count);

            if (count > 0) {
                auto packet = read_socket(unicast_fd);
                process_packet(std::move(packet.first), std::move(packet.second));
            }
        }


        void NUClearNetwork::announce() {

            // Check if any of our existing connections have timed out
            /* Mutex scope */ {
                std::lock_guard<std::mutex> lock(target_mutex);
                auto now = std::chrono::steady_clock::now();
                for (auto it = targets.begin(); it != targets.end();) {
                    if (now - it->last_update > std::chrono::seconds(2)) {

                        // Remove this, it timed out
                        leave_callback(it->name, it->target);
                        it = remove_target(it);
                    }
                    else {
                        ++it;
                    }
                }
            }

            // Send the packet
            ::sendto(
                unicast_fd, announce_packet.data(), announce_packet.size(), 0, &multicast_target, sizeof(sockaddr));
        }


        void NUClearNetwork::process_packet(sockaddr&& address, std::vector<char>&& payload) {

            // First validate this is a NUClear network packet we can read
            if (payload[0] == '\xE2' && payload[1] == '\x98' && payload[2] == '\xA2' && payload[3] == 0x02) {

                // This is a real packet! get our header information
                const PacketHeader& header = *reinterpret_cast<const PacketHeader*>(payload.data());

                // Cast into an IPV4, hopefully one day we will handle both
                const sockaddr_in& ipv4 = *reinterpret_cast<const sockaddr_in*>(&address);
                auto udp_key            = std::make_pair(ipv4.sin_addr.s_addr, ipv4.sin_port);

                // From here on, we are doing things with our target lists that if changed would make us sad
                std::lock_guard<std::mutex> target_lock(target_mutex);
                auto remote = udp_target.find(udp_key);

                switch (header.type) {
                    case ANNOUNCE: {
                        // This is an announce packet!
                        const AnnouncePacket& announce = *reinterpret_cast<const AnnouncePacket*>(payload.data());

                        // They're new!
                        if (remote == udp_target.end()) {
                            std::string name(&announce.name, payload.size() - sizeof(AnnouncePacket));

                            // Say hi back!
                            ::sendto(unicast_fd,
                                     announce_packet.data(),
                                     announce_packet.size(),
                                     0,
                                     &address,
                                     sizeof(sockaddr));

                            targets.emplace_front(name, address);
                            auto it = targets.begin();
                            udp_target.insert(std::make_pair(udp_key, it));
                            name_target.insert(std::make_pair(name, it));

                            join_callback(name, address);
                        }
                        // They're old but at least they're not timing out
                        else {
                            remote->second->last_update = std::chrono::steady_clock::now();
                        }
                    } break;
                    case LEAVE: {

                        // Goodbye!
                        if (remote != udp_target.end()) {

                            // Need to call leave before they actually go because otherwise we delete their information
                            leave_callback(remote->second->name, remote->second->target);

                            remove_target(remote->second);
                        }

                    } break;
                    case DATA: {

                        // It's a data packet
                        const DataPacket& packet = *reinterpret_cast<const DataPacket*>(payload.data());

                        // Check if we know who this is and if we don't know them, ignore
                        if (remote != udp_target.end()) {

                            // We got a packet from them recently
                            remote->second->last_update = std::chrono::steady_clock::now();

                            // If this is a solo packet (in a single chunk)
                            if (packet.packet_count == 1) {

                                // Copy our data into a vector
                                std::vector<char> out(&packet.data,
                                                      &packet.data + payload.size() - sizeof(DataPacket) + 1);

                                // If this is a reliable packet, send an ack back
                                if (packet.reliable) {
                                    // This response is easy since there is only one packet
                                    ACKPacket response;
                                    response.type         = ACK;
                                    response.packet_id    = packet.packet_id;
                                    response.packet_count = packet.packet_count;
                                    response.packets      = 1;
                                    ::sendto(unicast_fd,
                                             &response,
                                             sizeof(response),
                                             0,
                                             &remote->second->target,
                                             sizeof(sockaddr));
                                }

                                packet_callback(*remote->second, packet.hash, std::move(out));
                            }
                            else {
                                std::lock_guard<std::mutex> lock(remote->second->assembly_mutex);

                                // Grab the payload and put it in our list of assembly targets
                                auto& assembly                     = remote->second->assembly;
                                auto& assembler                    = assembly[packet.packet_id];
                                assembler.first                    = std::chrono::steady_clock::now();
                                assembler.second[packet.packet_no] = std::move(payload);

                                // Create and send our ACK packet if this is a reliable transmission
                                if (packet.reliable) {
                                    // A basic ack has room for 8 packets and we need 1 extra byte for each 8 additional
                                    // packets
                                    std::vector<char> r(sizeof(ACKPacket) + (packet.packet_count / 8), 0);
                                    ACKPacket& response   = *reinterpret_cast<ACKPacket*>(r.data());
                                    response              = ACKPacket();
                                    response.type         = ACK;
                                    response.packet_id    = packet.packet_id;
                                    response.packet_count = packet.packet_count;

                                    // Set the bits for the packets we have received
                                    for (auto& p : assembler.second) {
                                        (&response.packets)[p.first / 8] |= uint8_t(1 << (p.first % 8));
                                    }

                                    // Send the packet
                                    ::sendto(
                                        unicast_fd, r.data(), r.size(), 0, &remote->second->target, sizeof(sockaddr));
                                }

                                // Check to see if we have enough to assemble the whole thing
                                if (assembler.second.size() == packet.packet_count) {
                                    // We know the data is at least over 1500
                                    std::vector<char> out;
                                    out.reserve(4096);

                                    int i = 0;
                                    for (auto& packet : assembler.second) {
                                        if (packet.first != i++) {
                                            // TODO THIS IS A CORRUPTED PACKET
                                            // TODO you should keep track of the sequence number from the remote and purge stuff you know is bad
                                            break;
                                        }
                                        else {
                                            const DataPacket& p = *reinterpret_cast<DataPacket*>(packet.second.data());
                                            out.insert(
                                                out.end(),
                                                &p.data,
                                                &p.data + packet.second.size() - sizeof(DataPacket) + 1);
                                        }
                                    }
                                    
                                    packet_callback(*remote->second, packet.hash, std::move(out));
                                    
                                    // We have completed this packet, discard the data
                                    assembly.erase(assembly.find(packet.packet_id));
                                }
                                else if (assembler.second.size() > packet.packet_count) {
                                    // TODO THIS IS A CORRUPTED PACKET
                                }

                                // TODO cleanup old packet assembly here to free memory
                            }
                        }
                    } break;
                    case ACK: {

                        // It's an ack packet
                        const ACKPacket& packet = *reinterpret_cast<const ACKPacket*>(payload.data());

                        // Check if we know who this is and if we don't know them, ignore
                        if (remote != udp_target.end()) {

                            // We got a packet from them recently
                            remote->second->last_update = std::chrono::steady_clock::now();

                            std::cout << remote->second->name << " ACK Packet: " << packet.packet_id << " ";
                            for (int i = 0; i < packet.packet_count; ++i) {
                                std::cout << (((&packet.packets)[i / 8] & uint8_t(1 << (i % 8))) != 0);
                            }
                            std::cout << std::endl;
                        }


                        // TODO Store the updated ack status in our outoging reliable message queue for this host and if
                        // it's totally sent, we can drop the queue
                    } break;
                }
            }
        }


        std::vector<int> NUClearNetwork::listen_fds() {
            return std::vector<int>({unicast_fd, multicast_fd});
        }


        void NUClearNetwork::send(const std::array<uint64_t, 2>& hash,
                                  const std::vector<char>& payload,
                                  const std::string& target,
                                  bool reliable) {

            // Our packet we are sending
            msghdr message;
            std::memset(&message, 0, sizeof(msghdr));

            iovec data[2];
            message.msg_iov    = data;
            message.msg_iovlen = 2;

            // The first element in our iovec is the header
            DataPacket header;
            data[0].iov_base = reinterpret_cast<char*>(&header);
            data[0].iov_len  = sizeof(DataPacket) - 1;

            // Some references for easy access to our data memory
            auto& base = data[1].iov_base;
            auto& len  = data[1].iov_len;

            // Set some common information for the header
            header.type         = DATA;
            header.packet_id    = ++packet_id_source;
            header.packet_no    = 0;
            header.packet_count = uint16_t((payload.size() / MAX_UDP_PAYLOAD_LENGTH) + 1);
            header.reliable     = reliable;
            header.hash         = hash;

            // TODO if reliable is true we need to make sure we can retransmit on request

            // Loop through our chunks
            for (size_t i = 0; i < payload.size(); i += MAX_UDP_PAYLOAD_LENGTH) {

                // Store our payload information for this chunk
                base = const_cast<char*>(payload.data() + i);
                len  = (i + MAX_UDP_PAYLOAD_LENGTH) < payload.size() ? MAX_UDP_PAYLOAD_LENGTH
                                                                    : payload.size() % MAX_UDP_PAYLOAD_LENGTH;

                // Send multicast
                if (target.empty()) {
                    message.msg_name    = &multicast_target;
                    message.msg_namelen = sizeof(sockaddr);

                    // Send the packet
                    sendmsg(unicast_fd, &message, 0);
                }
                // Send unicast
                else {
                    std::lock_guard<std::mutex> lock(target_mutex);
                    auto send_to = name_target.equal_range(target);

                    for (auto it = send_to.first; it != send_to.second; ++it) {

                        message.msg_name    = &it->second->target;
                        message.msg_namelen = sizeof(sockaddr);

                        // Send the packet
                        sendmsg(unicast_fd, &message, 0);
                    }
                }

                // Increment to send the next packet
                ++header.packet_no;
            }
        }
        // void NetworkController::udp_send(const NetworkEmit& emit) {

        //     // Make our message struct
        //     msghdr message;
        //     std::memset(&message, 0, sizeof(msghdr));

        //     // Create our iovec
        //     iovec data[2];
        //     message.msg_iov    = data;
        //     message.msg_iovlen = 2;

        //     // The first element in our iovec is the header
        //     DataPacket header;
        //     data[0].iov_base = reinterpret_cast<char*>(&header);
        //     data[0].iov_len  = sizeof(DataPacket) - 1;

        //     // Some references for easy access to our data memory
        //     auto& base = data[1].iov_base;
        //     auto& len  = data[1].iov_len;

        //     // Set some common information for the header
        //     header.type         = DATA;
        //     header.packet_id    = ++packet_id_source;
        //     header.packet_no    = 0;
        //     header.packet_count = uint16_t((emit.payload.size() / MAX_UDP_PAYLOAD_LENGTH) + 1);
        //     header.multicast    = emit.target.empty();
        //     header.hash         = emit.hash;

        //     // Loop through our chunks
        //     for (size_t i = 0; i < emit.payload.size(); i += MAX_UDP_PAYLOAD_LENGTH) {

        //         // Store our payload information for this chunk
        //         base = const_cast<char*>(emit.payload.data() + i);
        //         len  = (i + MAX_UDP_PAYLOAD_LENGTH) < emit.payload.size() ? MAX_UDP_PAYLOAD_LENGTH
        //                                                                  : emit.payload.size() %
        //                                                                  MAX_UDP_PAYLOAD_LENGTH;

        //         // Work out our header length
        //         header.length = uint32_t(len + sizeof(DataPacket) - sizeof(PacketHeader) - 1);

        //         // Send multicast
        //         if (emit.target.empty()) {

        //             // Multicast address
        //             sockaddr_in target;
        //             std::memset(&target, 0, sizeof(sockaddr_in));
        //             target.sin_family = AF_INET;
        //             inet_pton(AF_INET, multicast_group.c_str(), &target.sin_addr);
        //             target.sin_port = htons(multicast_port);

        //             message.msg_name    = reinterpret_cast<sockaddr*>(&target);
        //             message.msg_namelen = sizeof(sockaddr_in);

        //             // Send the packet
        //             sendmsg(unicast_fd, &message, 0);
        //         }
        //         // Send unicast
        //         else {
        //             auto send_to = name_target.equal_range(emit.target);

        //             for (auto it = send_to.first; it != send_to.second; ++it) {

        //                 // Unicast address
        //                 sockaddr_in target;
        //                 std::memset(&target, 0, sizeof(sockaddr_in));
        //                 target.sin_family      = AF_INET;
        //                 target.sin_addr.s_addr = htonl(it->second->address);
        //                 target.sin_port        = htons(it->second->udp_port);

        //                 message.msg_name    = reinterpret_cast<sockaddr*>(&target);
        //                 message.msg_namelen = sizeof(sockaddr_in);

        //                 // Send the packet
        //                 sendmsg(unicast_fd, &message, 0);
        //             }
        //         }

        //         // Increment to send the next packet
        //         ++header.packet_no;
        //     }
        // }


        // void NUClearNetwork::packet_handler(const UDP::Packet& packet) {

        //     // TODO work out if this is us sending multicast to ourselves and if so return
        //     // Indicators, udp_port = our_udp_port
        //     // packet.remote.address == ours (? but how do we know our address?)

        //     // Work out what type of packet this is
        //     const PacketHeader& header =
        //         *reinterpret_cast<const PacketHeader*>(packet.payload.data());

        //     if (header.type == ANNOUNCE) {

        //         const AnnouncePacket& announce =
        //             *reinterpret_cast<const AnnouncePacket*>(packet.payload.data());

        //         std::string new_name = &announce.name;
        //         int new_tcp_port     = announce.tcp_port;
        //         int new_udp_port     = announce.udp_port;

        //         // Make sure this packet isn't suspect
        //         if (packet.remote.port == new_udp_port) {

        //             // Make sure this is not us
        //             if (!(name == new_name && tcp_port == new_tcp_port && udp_port == new_udp_port)) {

        //                 // Check we do not already have this client connected
        //                 if (udp_target.find(std::make_pair(packet.remote.address, new_udp_port)) ==
        //                 udp_target.end())
        //                 {

        //                     sockaddr_in local;
        //                     std::memset(&local, 0, sizeof(sockaddr_in));
        //                     local.sin_family      = AF_INET;
        //                     local.sin_port        = htons(tcp_port);
        //                     local.sin_addr.s_addr = htonl(INADDR_ANY);

        //                     sockaddr_in remote;
        //                     std::memset(&remote, 0, sizeof(sockaddr_in));
        //                     remote.sin_family      = AF_INET;
        //                     remote.sin_port        = htons(new_tcp_port);
        //                     remote.sin_addr.s_addr = htonl(packet.remote.address);

        //                     // Open a TCP connection
        //                     util::FileDescriptor tcp_fd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        //                     ::connect(tcp_fd, reinterpret_cast<sockaddr*>(&remote), sizeof(sockaddr));
        //                     // TODO this might get closed or refused or something else

        //                     // Make a data vector of the correct size and default values
        //                     std::vector<char> announce_packet(sizeof(AnnouncePacket) + name.size());
        //                     AnnouncePacket* p =
        //                         reinterpret_cast<AnnouncePacket*>(announce_packet.data());
        //                     *p = AnnouncePacket();

        //                     // Make an announce packet
        //                     p->type     = ANNOUNCE;
        //                     p->tcp_port = tcp_port;
        //                     p->udp_port = udp_port;

        //                     // Length is the size without the header
        //                     p->length =
        //                         uint32_t(sizeof(AnnouncePacket) + name.size() -
        //                         sizeof(PacketHeader));

        //                     // Copy our name over
        //                     std::memcpy(&p->name, name.c_str(), name.size() + 1);

        //                     // Copy our packet data and name over
        //                     ::send(tcp_fd, announce_packet.data(), announce_packet.size(), 0);

        //                     // Insert our new element
        //                     auto it = targets.emplace(targets.end(),
        //                                               new_name,
        //                                               packet.remote.address,
        //                                               new_tcp_port,
        //                                               new_udp_port,
        //                                               tcp_fd.release());
        //                     name_target.insert(std::make_pair(new_name, it));
        //                     udp_target.insert(std::make_pair(std::make_pair(packet.remote.address, new_udp_port),
        //                     it));
        //                     tcp_target.insert(std::make_pair(it->tcp_fd, it));

        //                     // Start our connected handle
        //                     it->handle =
        //                         on<IO, Sync<NetworkController>>(it->tcp_fd, IO::READ | IO::ERROR | IO::CLOSE)
        //                             .then("Network TCP Handler", [this](const IO::Event& e) { tcp_handler(e); });

        //                     // emit a message that says who connected
        //                     auto j      = std::make_unique<message::NetworkJoin>();
        //                     j->name     = &announce.name;
        //                     j->address  = packet.remote.address;
        //                     j->tcp_port = announce.tcp_port;
        //                     j->udp_port = announce.udp_port;
        //                     emit(j);
        //                 }
        //             }
        //         }
        //     }
        //     else if (header.type == DATA) {

        //         // Work out who our remote is
        //         auto remote = udp_target.find(std::make_pair(packet.remote.address, packet.remote.port));

        //         // Check if we know who this is and if we don't know them, ignore
        //         if (remote != udp_target.end()) {

        //             const DataPacket& p = *reinterpret_cast<const
        //             DataPacket*>(packet.payload.data());

        //             // If this is a solo packet (in a single chunk)
        //             if (p.packet_no == 0 && p.packet_count == 1) {

        //                 // Copy our data into a vector
        //                 std::vector<char> payload(
        //                     &p.data,
        //                     &p.data + p.length - sizeof(DataPacket) + sizeof(PacketHeader) +
        //                     1);

        //                 // Construct our NetworkSource information
        //                 dsl::word::NetworkSource src;
        //                 src.name      = remote->second->name;
        //                 src.address   = remote->second->address;
        //                 src.port      = remote->second->udp_port;
        //                 src.reliable  = true;
        //                 src.multicast = p.multicast;

        //                 // Store in our thread local cache
        //                 dsl::store::ThreadStore<std::vector<char>>::value        = &payload;
        //                 dsl::store::ThreadStore<dsl::word::NetworkSource>::value = &src;

        //                 /* Mutex Scope */ {
        //                     // Lock our reaction mutex
        //                     std::lock_guard<std::mutex> lock(reaction_mutex);

        //                     // Find interested reactions
        //                     auto rs = reactions.equal_range(p.hash);

        //                     // Execute on our interested reactions
        //                     for (auto it = rs.first; it != rs.second; ++it) {
        //                         auto task = it->second->get_task();
        //                         if (task) {
        //                             powerplant.submit(std::move(task));
        //                         }
        //                     }
        //                 }

        //                 // Clear our cache
        //                 dsl::store::ThreadStore<std::vector<char>>::value        = nullptr;
        //                 dsl::store::ThreadStore<dsl::word::NetworkSource>::value = nullptr;
        //             }
        //             else {
        //                 std::lock_guard<std::mutex> lock(remote->second->buffer_mutex);

        //                 // Get our buffer
        //                 auto& buffer = remote->second->buffer;

        //                 // Check if we have too many packets
        //                 if (buffer.size() > MAX_NUM_UDP_ASSEMBLY) {
        //                     // Remove the oldest assembly target
        //                     auto oldest = buffer.begin();

        //                     for (auto it = buffer.begin(); it != buffer.end(); ++it) {
        //                         if (it->second.first < oldest->second.first) {
        //                             oldest = it;
        //                         }
        //                     }

        //                     buffer.erase(oldest);
        //                 }

        //                 auto& set = buffer[p.packet_id];
        //                 set.first = clock::now();

        //                 // Add our packet
        //                 set.second.push_back(packet.payload);

        //                 // If we are finished assemble and emit
        //                 if (set.second.size() == p.packet_count) {

        //                     // Our final payload
        //                     std::vector<char> payload;

        //                     // Sort the list
        //                     std::sort(set.second.begin(),
        //                               set.second.end(),
        //                               [](const std::vector<char>& a, const std::vector<char>& b) {
        //                                   const DataPacket& p_a =
        //                                       *reinterpret_cast<const DataPacket*>(a.data());
        //                                   const DataPacket& p_b =
        //                                       *reinterpret_cast<const DataPacket*>(b.data());
        //                                   return p_a.packet_no < p_b.packet_no;
        //                               });

        //                     // Copy the data across
        //                     for (auto& v : set.second) {
        //                         payload.insert(payload.end(), v.begin() + sizeof(DataPacket) - 1,
        //                         v.end());
        //                     }

        //                     // TODO send the packet via callback

        //                     // Erase this from the list
        //                     buffer.erase(buffer.find(p.packet_id));
        //                 }
        //             }
        //         }
        //     }
        // }

    }  // namespace network
}  // namespace extension
}  // namespace NUClear
