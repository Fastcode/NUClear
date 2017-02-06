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

#include <netdb.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <algorithm>
#include <cerrno>

namespace NUClear {
namespace extension {
    namespace network {

        size_t socket_size(const util::network::sock_t& s) {
            switch (s.sock.sa_family) {
                case AF_INET: return sizeof(sockaddr_in);
                case AF_INET6: return sizeof(sockaddr_in6);
                default: return -1;
            }
        }

        NUClearNetwork::NUClearNetwork()
            : name("")
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


        void NUClearNetwork::set_join_callback(std::function<void(const NetworkTarget&)> f) {
            join_callback = f;
        }


        void NUClearNetwork::set_leave_callback(std::function<void(const NetworkTarget&)> f) {
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

            // Create the "join any" address for this address family
            sock_t address = multicast_target;

            // IPv4
            if (address.sock.sa_family == AF_INET) {
                address.ipv4.sin_family      = AF_INET;
                address.ipv4.sin_addr.s_addr = htonl(INADDR_ANY);
                address.ipv4.sin_port        = 0;
            }
            // IPv6
            else if (address.sock.sa_family == AF_INET6) {
                address.ipv6.sin6_family = AF_INET6;
                address.ipv6.sin6_addr   = IN6ADDR_ANY_INIT;
                address.ipv6.sin6_port   = 0;
            }

            // Open a socket with the same family as our multicast target
            unicast_fd = ::socket(address.sock.sa_family, SOCK_DGRAM, IPPROTO_UDP);
            if (unicast_fd < 0) {
                throw std::system_error(network_errno, std::system_category(), "Unable to open the UDP socket");
            }

            // Bind to the address, and if we fail throw an error
            if (::bind(unicast_fd, &address.sock, socket_size(address))) {
                throw std::system_error(
                    network_errno, std::system_category(), "Unable to bind the UDP socket to the port");
            }
        }


        void NUClearNetwork::open_multicast() {

            // Rather than listen on the multicast address directly, we join everything so
            // our traffic isn't filtered allowing us to get multicast traffic from multiple devices
            sock_t address = multicast_target;

            // IPv4
            if (address.sock.sa_family == AF_INET) {
                address.ipv4.sin_addr.s_addr = htonl(INADDR_ANY);
            }
            // IPv6
            else if (address.sock.sa_family == AF_INET6) {
                address.ipv6.sin6_addr = IN6ADDR_ANY_INIT;
            }

            // Make our socket
            multicast_fd = ::socket(address.sock.sa_family, SOCK_DGRAM, IPPROTO_UDP);
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
            if (::bind(multicast_fd, &address.sock, socket_size(address))) {
                throw std::system_error(network_errno, std::system_category(), "Unable to bind the UDP socket");
            }

            // Our multicast join request will depend on protocol version
            if (multicast_target.sock.sa_family == AF_INET) {

                // Set the multicast address we are listening on
                ip_mreq mreq;
                mreq.imr_multiaddr = multicast_target.ipv4.sin_addr;

                // Join the multicast group on all the interfaces that support it
                for (auto& iface : util::network::get_interfaces()) {

                    if (iface.ip.sock.sa_family == AF_INET && iface.flags.multicast) {

                        // Set our interface address
                        mreq.imr_interface = iface.ip.ipv4.sin_addr;

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
            else if (multicast_target.sock.sa_family == AF_INET6) {

                // Set the multicast address we are listening on
                ipv6_mreq mreq;
                mreq.ipv6mr_multiaddr = multicast_target.ipv6.sin6_addr;

                // Join the multicast group on all the interfaces that support it
                for (auto& iface : util::network::get_interfaces()) {

                    if (iface.ip.sock.sa_family == AF_INET && iface.flags.multicast) {

                        // Get the interface for this
                        mreq.ipv6mr_interface = if_nametoindex(iface.name.c_str());

                        // Join our multicast group
                        if (::setsockopt(multicast_fd,
                                         IPPROTO_IPV6,
                                         IPV6_JOIN_GROUP,
                                         reinterpret_cast<char*>(&mreq),
                                         sizeof(ipv6_mreq))
                            < 0) {
                            throw std::system_error(network_errno,
                                                    std::system_category(),
                                                    "There was an error while attempting to join the multicast group");
                        }
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
                ::sendto(unicast_fd, &packet, sizeof(packet), 0, &multicast_target.sock, socket_size(multicast_target));
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

            // Close our existing FDs if they exist
            shutdown();
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

            // Setup some hints for what our address is
            addrinfo hints;
            memset(&hints, 0, sizeof hints);  // make sure the struct is empty
            hints.ai_family   = AF_UNSPEC;    // don't care about IPv4 or IPv6
            hints.ai_socktype = SOCK_DGRAM;   // using udp datagrams

            // Get our info on this address
            addrinfo* servinfo;
            ::getaddrinfo(group.c_str(), std::to_string(port).c_str(), &hints, &servinfo);

            // Check if we have any addresses to work with
            if (servinfo == nullptr) {
                throw std::runtime_error(std::string("The multicast address provided (") + group + ") was invalid");
            }

            // Clear our struct
            std::memset(&multicast_target, 0, sizeof(multicast_target));

            // The list is actually a linked list of valid addresses
            // The address we choose is in the following priority, IPv4, IPv6, Other
            for (addrinfo* p = servinfo; p != nullptr; p = p->ai_next) {

                // If we find an IPv4 address, prefer that
                if (servinfo->ai_family == AF_INET) {
                    auto& addr = *reinterpret_cast<const sockaddr_in*>(servinfo->ai_addr);

                    // Check this address is multicast in the Administratively-scoped group
                    // (starts with 1110 1111)
                    if ((htonl(addr.sin_addr.s_addr) & 0xEF000000) == 0xEF000000) {

                        // Clear and set our struct
                        std::memset(&multicast_target, 0, sizeof(multicast_target));
                        std::memcpy(&multicast_target, servinfo->ai_addr, servinfo->ai_addrlen);

                        // We prefer IPv4 so use it and stop looking
                        break;
                    }
                }
                // If we find an IPv6 address now we just use that
                else if (servinfo->ai_family == AF_INET6) {
                    auto& addr = *reinterpret_cast<const sockaddr_in6*>(servinfo->ai_addr);

                    // Check the address is multicast (starts with 0xFF)
                    if (addr.sin6_addr.s6_addr[0] == 0xFF) {

                        // Clear and set our struct
                        std::memset(&multicast_target, 0, sizeof(multicast_target));
                        std::memcpy(&multicast_target, servinfo->ai_addr, servinfo->ai_addrlen);
                    }
                }
            }

            ::freeaddrinfo(servinfo);

            // If we couldn't find a useable address die
            if (multicast_target.sock.sa_family != AF_INET && multicast_target.sock.sa_family != AF_INET6) {
                throw std::runtime_error(std::string("The network address provided (") + group
                                         + ") was not a valid multicast address");
            }

            // Build our announce packet
            announce_packet.resize(sizeof(AnnouncePacket) + name.size(), 0);
            AnnouncePacket& pkt = *reinterpret_cast<AnnouncePacket*>(announce_packet.data());
            pkt                 = AnnouncePacket();
            pkt.type            = ANNOUNCE;
            std::memcpy(&pkt.name, name.c_str(), name.size());

            // Open our unicast socket and then our multicast one
            open_unicast();
            open_multicast();
        }


        std::pair<util::network::sock_t, std::vector<char>> NUClearNetwork::read_socket(int fd) {

            // Allocate a vector that can hold a datagram
            std::vector<char> payload(1500);
            iovec iov;
            iov.iov_base = payload.data();
            iov.iov_len  = payload.size();

            // Who we are receiving from
            sock_t from;
            std::memset(&from, 0, sizeof(from));

            // Setup our message header to receive
            msghdr mh;
            memset(&mh, 0, sizeof(msghdr));
            mh.msg_name    = &from;
            mh.msg_namelen = sizeof(from);
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
                        leave_callback(*it);
                        it = remove_target(it);
                    }
                    else {
                        ++it;
                    }
                }
            }

            // Send the packet
            if (::sendto(unicast_fd,
                         announce_packet.data(),
                         announce_packet.size(),
                         0,
                         &multicast_target.sock,
                         socket_size(multicast_target))
                < 0) {
                throw std::system_error(
                    network_errno, std::system_category(), "Network error when sending the announce packet");
            }
        }


        void NUClearNetwork::process_packet(sock_t&& address, std::vector<char>&& payload) {

            // First validate this is a NUClear network packet we can read (a version 2 NUClear packet)
            if (payload[0] == '\xE2' && payload[1] == '\x98' && payload[2] == '\xA2' && payload[3] == 0x02) {

                // This is a real packet! get our header information
                const PacketHeader& header = *reinterpret_cast<const PacketHeader*>(payload.data());

                // Work out our key for this target based on their address
                // TODO CURRENTLY STUCK ON IPV4 ONLY

                // Cast into an IPV4, hopefully one day we will handle both
                const sockaddr_in& ipv4 = *reinterpret_cast<const sockaddr_in*>(&address);
                auto udp_key            = std::make_pair(ipv4.sin_addr.s_addr, ipv4.sin_port);

                // From here on, we are doing things with our target lists that if changed would make us sad
                std::lock_guard<std::mutex> target_lock(target_mutex);
                auto remote = udp_target.find(udp_key);

                switch (header.type) {

                    // A packet announcing that a user is on the network
                    case ANNOUNCE: {
                        // This is an announce packet!
                        const AnnouncePacket& announce = *reinterpret_cast<const AnnouncePacket*>(payload.data());

                        // They're new!
                        if (remote == udp_target.end()) {
                            std::string name(&announce.name, payload.size() - sizeof(AnnouncePacket));

                            // Make who we are sending it to into a useable address
                            sock_t& to = remote->second->target;

                            // Say hi back!
                            ::sendto(unicast_fd,
                                     announce_packet.data(),
                                     announce_packet.size(),
                                     0,
                                     &to.sock,
                                     socket_size(to));

                            targets.emplace_front(name, address);
                            auto it = targets.begin();
                            udp_target.insert(std::make_pair(udp_key, it));
                            name_target.insert(std::make_pair(name, it));

                            join_callback(*it);
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
                            leave_callback(*remote->second);

                            remove_target(remote->second);
                        }

                    } break;

                    // A packet containing data
                    case DATA: {

                        // It's a data packet
                        const DataPacket& packet = *reinterpret_cast<const DataPacket*>(payload.data());

                        // If the packet is obviously corrupt, drop it and since we didn't ack it it'll be resent if
                        // it's important
                        if (packet.packet_no > packet.packet_count) {
                            return;
                        }

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

                                    // Make who we are sending it to into a useable address
                                    sock_t& to = remote->second->target;

                                    ::sendto(unicast_fd, &response, sizeof(response), 0, &to.sock, socket_size(to));
                                }

                                packet_callback(*remote->second, packet.hash, std::move(out));
                            }
                            else {
                                std::lock_guard<std::mutex> lock(remote->second->assemblers_mutex);

                                // Grab the payload and put it in our list of assemblers targets
                                auto& assemblers = remote->second->assemblers;
                                auto& assembler  = assemblers[packet.packet_id];

                                // First check that our cache isn't super corrupted by ensuring that our last packet in
                                // our list isn't after the number of packets we have
                                if (!assembler.second.empty()
                                    && std::next(assembler.second.end(), -1)->first >= packet.packet_count) {

                                    // If so, we need to purge our cache and if this was a reliable packet, send a NACK
                                    // back for all the packets we thought we had
                                    // We don't know if we have any packets except the one we just got
                                    if (packet.reliable) {

                                        // A basic ack has room for 8 packets and we need 1 extra byte for each 8
                                        // additional
                                        // packets
                                        std::vector<char> r(sizeof(NACKPacket) + (packet.packet_count / 8), 0);
                                        NACKPacket& response  = *reinterpret_cast<NACKPacket*>(r.data());
                                        response              = NACKPacket();
                                        response.type         = NACK;
                                        response.packet_id    = packet.packet_id;
                                        response.packet_count = packet.packet_count;

                                        // Set the bits for the packets we thought we received
                                        for (auto& p : assembler.second) {
                                            (&response.packets)[p.first / 8] |= uint8_t(1 << (p.first % 8));
                                        }

                                        // Ensure the bit for this packet isn't NACKed
                                        (&response.packets)[packet.packet_no / 8] &=
                                            ~uint8_t(1 << (packet.packet_no % 8));

                                        // Make who we are sending it to into a useable address
                                        sock_t& to = remote->second->target;

                                        // Send the packet
                                        ::sendto(unicast_fd, r.data(), r.size(), 0, &to.sock, socket_size(to));
                                    }

                                    assembler.second.clear();
                                }

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
                                    
                                    // Make who we are sending it to into a useable address
                                    sock_t& to = remote->second->target;
                                    
                                    // Send the packet
                                    ::sendto(unicast_fd, r.data(), r.size(), 0, &to.sock, socket_size(to));
                                }
                                
                                // Check to see if we have enough to assemble the whole thing
                                if (assembler.second.size() == packet.packet_count) {

                                    // Work out exactly how much data we will need first so we only need one allocation
                                    size_t payload_size = 0;
                                    int packet_index    = 0;
                                    for (auto& packet : assembler.second) {
                                        if (packet.first != packet_index++) {
                                            // TODO this data was bad, erase it or something
                                            return;
                                        }
                                        else {
                                            payload_size += packet.second.size() - sizeof(DataPacket) + 1;
                                        }
                                    }

                                    // Read in our data
                                    std::vector<char> out;
                                    out.reserve(payload_size);
                                    for (auto& packet : assembler.second) {
                                        const DataPacket& p = *reinterpret_cast<DataPacket*>(packet.second.data());
                                        out.insert(out.end(),
                                                   &p.data,
                                                   &p.data + packet.second.size() - sizeof(DataPacket) + 1);
                                    }

                                    // Send our assembled data packet
                                    packet_callback(*remote->second, packet.hash, std::move(out));

                                    // We have completed this packet, discard the data
                                    assemblers.erase(assemblers.find(packet.packet_id));
                                }
                                else if (assembler.second.size() > packet.packet_count) {
                                    // TODO THIS IS A CORRUPTED PACKET IT LOOPED OR SOMETHING!!
                                    // ALSO THERE IS A POSSIBLITY THAT WE ACKED INCORRECTLY EARLIER
                                }

                                // TODO cleanup old packet assemblers here to free memory
                            }
                        }
                    } break;

                    // Packet acknowledging the receipt of a packet of data
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
                        // it's totally sent, we can drop the queue item
                    } break;

                    // Packet requesting a retransmission of some corrupt data
                    case NACK: {
                        // It's a nack packet
                        const NACKPacket& packet = *reinterpret_cast<const NACKPacket*>(payload.data());

                        // Check if we know who this is and if we don't know them, ignore
                        if (remote != udp_target.end()) {

                            // We got a packet from them recently
                            remote->second->last_update = std::chrono::steady_clock::now();

                            // Find this packet in our sending queue

                            // Resend the packets that are NACKed
                            std::cout << remote->second->name << " NACK Packet: " << packet.packet_id << " ";
                            for (int i = 0; i < packet.packet_count; ++i) {
                                std::cout << (((&packet.packets)[i / 8] & uint8_t(1 << (i % 8))) != 0);
                            }
                            std::cout << std::endl;
                        }
                    }
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

            // Loop through our chunks
            for (size_t i = 0; i < payload.size(); i += MAX_UDP_PAYLOAD_LENGTH) {

                // Store our payload information for this chunk
                base = const_cast<char*>(payload.data() + i);
                len  = (i + MAX_UDP_PAYLOAD_LENGTH) < payload.size() ? MAX_UDP_PAYLOAD_LENGTH
                                                                    : payload.size() % MAX_UDP_PAYLOAD_LENGTH;

                // Send multicast
                if (target.empty()) {
                    message.msg_name    = &multicast_target;
                    message.msg_namelen = socket_size(multicast_target);

                    // Send the packet
                    sendmsg(unicast_fd, &message, 0);
                }
                // Send unicast
                else {
                    std::lock_guard<std::mutex> lock(target_mutex);
                    auto send_to = name_target.equal_range(target);

                    for (auto it = send_to.first; it != send_to.second; ++it) {

                        message.msg_name    = &it->second->target;
                        message.msg_namelen = socket_size(it->second->target);

                        // Send the packet
                        sendmsg(unicast_fd, &message, 0);
                    }
                }

                // Increment to send the next packet
                ++header.packet_no;
            }
        }

    }  // namespace network
}  // namespace extension
}  // namespace NUClear
