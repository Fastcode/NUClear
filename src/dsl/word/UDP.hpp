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

#ifndef NUCLEAR_DSL_WORD_UDP_HPP
#define NUCLEAR_DSL_WORD_UDP_HPP

#include "../../PowerPlant.hpp"
#include "../../util/FileDescriptor.hpp"
#include "../../util/network/get_interfaces.hpp"
#include "../../util/platform.hpp"
#include "IO.hpp"

namespace NUClear {
namespace dsl {
    namespace word {

        /**
         * @brief
         *  This allows a reaction to be triggered based on UDP activity originating from external sources, or UDP
         *  emissions within the system.
         *
         * @details
         *  @code on<UDP>(port) @endcode
         *  When a connection is identified on the assigned port, the associated reaction will be triggered.  The
         *  request for a UDP based reaction can use a runtime argument to reference a specific port.  Note that the
         *  port reference can be changed during the systems execution phase.
         *
         *  @code on<UDP>() @endcode
         *  Should the port reference be omitted, then the system will bind to a currently unassigned port.
         *
         *  @code on<UDP, UDP>(port, port)  @endcode
         *  A reaction can also be triggered via activity on more than one port.
         *
         *  @code on<UDP:Broadcast>(port)
         *  on<UDP:Multicast>(multicast_address, port) @endcode
         *  If needed, this trigger can also listen for UDP activity such as broadcast and multicast.
         *
         *  These requests currently support IPv4 addressing.
         *
         * @par Implements
         *  Bind
         */
        struct UDP {

            struct Packet {
                Packet() : valid(false), remote(), local(), payload() {}

                /// If the packet is valid (it contains data)
                bool valid;

                /// The information about this packet's source
                struct Remote {
                    Remote() : address(0), port(0) {}
                    Remote(uint32_t addr, uint16_t port) : address(addr), port(port) {}

                    /// The address that the packet is from
                    uint32_t address;
                    /// The port that the packet is from
                    uint16_t port;
                } remote;

                /// The information about this packet's destination
                struct Local {
                    Local() : address(0), port(0) {}
                    Local(uint32_t addr, uint16_t port) : address(addr), port(port) {}

                    /// The address that the packet is to
                    uint32_t address;
                    /// The port that the packet is to
                    uint16_t port;
                } local;

                /// The data to be sent in the packet
                std::vector<char> payload;

                /// Our validator when returned for if we are a real packet
                operator bool() const {
                    return valid;
                }

                // We can cast ourselves to a reference type so long as
                // that reference type is plain old data
                template <typename T>
                operator std::enable_if_t<std::is_pod<T>::value, const T&>() {
                    return *reinterpret_cast<const T*>(payload.data());
                }
            };

            template <typename DSL>
            static inline std::tuple<in_port_t, fd_t> bind(const std::shared_ptr<threading::Reaction>& reaction,
                                                           in_port_t port = 0) {

                // Make our socket
                util::FileDescriptor fd = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
                if (fd < 0) {
                    throw std::system_error(
                        network_errno, std::system_category(), "We were unable to open the UDP socket");
                }

                // The address we will be binding to
                sockaddr_in address;
                memset(&address, 0, sizeof(sockaddr_in));
                address.sin_family      = AF_INET;
                address.sin_port        = htons(port);
                address.sin_addr.s_addr = htonl(INADDR_ANY);

                // Bind to the address, and if we fail throw an error
                if (::bind(fd, reinterpret_cast<sockaddr*>(&address), sizeof(sockaddr))) {
                    throw std::system_error(
                        network_errno, std::system_category(), "We were unable to bind the UDP socket to the port");
                }

                int yes = 1;
                // Include struct in_pktinfo in the message "ancilliary" control data
                if (setsockopt(fd, IPPROTO_IP, IP_PKTINFO, reinterpret_cast<const char*>(&yes), sizeof(yes)) < 0) {
                    throw std::system_error(network_errno,
                                            std::system_category(),
                                            "We were unable to flag the socket as getting ancillary data");
                }

                // Get the port we ended up listening on
                socklen_t len = sizeof(sockaddr_in);
                if (::getsockname(fd, reinterpret_cast<sockaddr*>(&address), &len) == -1) {
                    throw std::system_error(
                        network_errno, std::system_category(), "We were unable to get the port from the UDP socket");
                }
                port = ntohs(address.sin_port);

                // Generate a reaction for the IO system that closes on death
                fd_t cfd = fd;
                reaction->unbinders.push_back([](const threading::Reaction& r) {
                    r.reactor.emit<emit::Direct>(std::make_unique<operation::Unbind<IO>>(r.id));
                });
                reaction->unbinders.push_back([cfd](const threading::Reaction&) { close(cfd); });

                auto io_config =
                    std::make_unique<IOConfiguration>(IOConfiguration{fd.release(), IO::READ, std::move(reaction)});

                // Send our configuration out
                reaction->reactor.emit<emit::Direct>(io_config);

                // Return our handles and our bound port
                return std::make_tuple(port, cfd);
            }

            template <typename DSL>
            static inline Packet get(threading::Reaction& r) {

                // Get our filedescriptor from the magic cache
                auto event = IO::get<DSL>(r);

                // If our get is being run without an fd (something else triggered) then short circuit
                if (event.fd == 0) {
                    Packet p;
                    p.remote.address = INADDR_NONE;
                    p.remote.port    = 0;
                    p.local.address  = INADDR_NONE;
                    p.local.port     = 0;
                    p.valid          = false;
                    return p;
                }

                // Make a packet with 2k of storage (hopefully packets are smaller then this as most MTUs are around
                // 1500)
                Packet p;
                p.remote.address = INADDR_NONE;
                p.remote.port    = 0;
                p.local.address  = INADDR_NONE;
                p.local.port     = 0;
                p.valid          = false;
                p.payload.resize(2048);

                // Make some variables to hold our message header information
                char cmbuff[0x100] = {0};
                sockaddr_in from;
                memset(&from, 0, sizeof(sockaddr_in));
                iovec payload;
                payload.iov_base = p.payload.data();
                payload.iov_len  = static_cast<decltype(payload.iov_len)>(p.payload.size());

                // Make our message header to receive with
                msghdr mh;
                memset(&mh, 0, sizeof(msghdr));
                mh.msg_name       = reinterpret_cast<sockaddr*>(&from);
                mh.msg_namelen    = sizeof(sockaddr_in);
                mh.msg_control    = cmbuff;
                mh.msg_controllen = sizeof(cmbuff);
                mh.msg_iov        = &payload;
                mh.msg_iovlen     = 1;

                // Receive our message
                ssize_t received = recvmsg(event.fd, &mh, 0);

                // Iterate through control headers to get IP information
                in_addr_t our_addr = 0;
                for (cmsghdr* cmsg = CMSG_FIRSTHDR(&mh); cmsg != nullptr; cmsg = CMSG_NXTHDR(&mh, cmsg)) {
                    // ignore the control headers that don't match what we want
                    if (cmsg->cmsg_level == IPPROTO_IP && cmsg->cmsg_type == IP_PKTINFO) {

                        // Access the packet header information
                        in_pktinfo* pi = reinterpret_cast<in_pktinfo*>(reinterpret_cast<char*>(cmsg) + sizeof(*cmsg));
                        our_addr       = pi->ipi_addr.s_addr;

                        // We are done
                        break;
                    }
                }

                // Get the port this socket is listening on
                socklen_t len = sizeof(sockaddr_in);
                sockaddr_in address;
                if (::getsockname(event.fd, reinterpret_cast<sockaddr*>(&address), &len) == -1) {
                    throw std::system_error(
                        network_errno, std::system_category(), "We were unable to get the port from the UDP socket");
                }

                // if no error
                if (received > 0) {
                    p.valid          = true;
                    p.remote.address = ntohl(from.sin_addr.s_addr);
                    p.remote.port    = ntohs(from.sin_port);
                    p.local.address  = ntohl(our_addr);
                    p.local.port     = ntohs(address.sin_port);
                    p.payload.resize(size_t(received));
                }

                return p;
            }

            struct Broadcast {

                template <typename DSL>
                static inline std::tuple<in_port_t, fd_t> bind(const std::shared_ptr<threading::Reaction>& reaction,
                                                               in_port_t port = 0) {

                    // Make our socket
                    util::FileDescriptor fd = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
                    if (fd < 0) {
                        throw std::system_error(
                            network_errno, std::system_category(), "We were unable to open the UDP Broadcast socket");
                    }

                    // The address we will be binding to
                    sockaddr_in address;
                    memset(&address, 0, sizeof(sockaddr_in));
                    address.sin_family      = AF_INET;
                    address.sin_port        = htons(port);
                    address.sin_addr.s_addr = htonl(INADDR_ANY);

                    int yes = true;
                    // We are a broadcast socket
                    if (setsockopt(fd, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<char*>(&yes), sizeof(yes)) < 0) {
                        throw std::system_error(
                            network_errno, std::system_category(), "We were unable to set the socket as broadcast");
                    }
                    // Set that we reuse the address so more than one application can bind
                    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char*>(&yes), sizeof(yes)) < 0) {
                        throw std::system_error(
                            network_errno, std::system_category(), "We were unable to set reuse address on the socket");
                    }
                    // Include struct in_pktinfo in the message "ancilliary" control data
                    if (setsockopt(fd, IPPROTO_IP, IP_PKTINFO, reinterpret_cast<char*>(&yes), sizeof(yes)) < 0) {
                        throw std::system_error(network_errno,
                                                std::system_category(),
                                                "We were unable to flag the socket as getting ancillary data");
                    }

                    // Bind to the address, and if we fail throw an error
                    if (::bind(fd, reinterpret_cast<sockaddr*>(&address), sizeof(sockaddr))) {
                        throw std::system_error(
                            network_errno, std::system_category(), "We were unable to bind the UDP socket to the port");
                    }

                    // Get the port we ended up listening on
                    socklen_t len = sizeof(sockaddr_in);
                    if (::getsockname(fd, reinterpret_cast<sockaddr*>(&address), &len) == -1) {
                        throw std::system_error(network_errno,
                                                std::system_category(),
                                                "We were unable to get the port from the UDP socket");
                    }
                    port = ntohs(address.sin_port);

                    // Generate a reaction for the IO system that closes on death
                    fd_t cfd = fd;
                    reaction->unbinders.push_back([](const threading::Reaction& r) {
                        r.reactor.emit<emit::Direct>(std::make_unique<operation::Unbind<IO>>(r.id));
                    });
                    reaction->unbinders.push_back([cfd](const threading::Reaction&) { close(cfd); });

                    auto io_config =
                        std::make_unique<IOConfiguration>(IOConfiguration{fd.release(), IO::READ, std::move(reaction)});

                    // Send our configuration out
                    reaction->reactor.emit<emit::Direct>(io_config);

                    // Return our handles and our bound port
                    return std::make_tuple(port, cfd);
                }

                template <typename DSL>
                static inline Packet get(threading::Reaction& r) {
                    return UDP::get<DSL>(r);
                }
            };

            struct Multicast {

                template <typename DSL>
                static inline std::tuple<in_port_t, fd_t> bind(const std::shared_ptr<threading::Reaction>& reaction,
                                                               std::string multicast_group,
                                                               in_port_t port = 0) {

                    // Our multicast group address
                    sockaddr_in address;
                    std::memset(&address, 0, sizeof(address));
                    address.sin_family      = AF_INET;
                    address.sin_addr.s_addr = INADDR_ANY;
                    address.sin_port        = htons(port);

                    // Make our socket
                    util::FileDescriptor fd = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
                    if (fd < 0) {
                        throw std::system_error(
                            network_errno, std::system_category(), "We were unable to open the UDP socket");
                    }

                    int yes = true;
                    // Set that we reuse the address so more than one application can bind
                    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char*>(&yes), sizeof(yes)) < 0) {
                        throw std::system_error(
                            network_errno, std::system_category(), "We were unable to set reuse address on the socket");
                    }
                    // Include struct in_pktinfo in the message "ancilliary" control data
                    if (setsockopt(fd, IPPROTO_IP, IP_PKTINFO, reinterpret_cast<char*>(&yes), sizeof(yes)) < 0) {
                        throw std::system_error(network_errno,
                                                std::system_category(),
                                                "We were unable to flag the socket as getting ancillary data");
                    }

                    // Bind to the address
                    if (::bind(fd, reinterpret_cast<sockaddr*>(&address), sizeof(sockaddr))) {
                        throw std::system_error(
                            network_errno, std::system_category(), "We were unable to bind the UDP socket to the port");
                    }

                    // Store the port variable that was used
                    socklen_t len = sizeof(sockaddr_in);
                    if (::getsockname(fd, reinterpret_cast<sockaddr*>(&address), &len) == -1) {
                        throw std::system_error(network_errno,
                                                std::system_category(),
                                                "We were unable to get the port from the UDP socket");
                    }
                    port = ntohs(address.sin_port);

                    // Get all the network interfaces that support multicast
                    std::vector<uint32_t> addresses;
                    for (auto& iface : util::network::get_interfaces()) {
                        // We receive on broadcast addresses and we don't want loopback or point to point
                        if (iface.flags.multicast && iface.ip.sock.sa_family == AF_INET) {
                            auto& i = *reinterpret_cast<const sockaddr_in*>(&iface.ip);
                            addresses.push_back(i.sin_addr.s_addr);
                        }
                    }

                    for (auto& ad : addresses) {

                        // Our multicast join request
                        ip_mreq mreq;
                        memset(&mreq, 0, sizeof(mreq));
                        inet_pton(AF_INET, multicast_group.c_str(), &mreq.imr_multiaddr);
                        mreq.imr_interface.s_addr = ad;

                        // Join our multicast group
                        if (setsockopt(
                                fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, reinterpret_cast<char*>(&mreq), sizeof(ip_mreq))
                            < 0) {
                            throw std::system_error(network_errno,
                                                    std::system_category(),
                                                    "There was an error while attempting to join the multicast group");
                        }
                    }

                    // Generate a reaction for the IO system that closes on death
                    fd_t cfd = fd;
                    reaction->unbinders.push_back([](const threading::Reaction& r) {
                        r.reactor.emit<emit::Direct>(std::make_unique<operation::Unbind<IO>>(r.id));
                    });
                    reaction->unbinders.push_back([cfd](const threading::Reaction&) { close(cfd); });

                    auto io_config =
                        std::make_unique<IOConfiguration>(IOConfiguration{fd.release(), IO::READ, std::move(reaction)});

                    // Send our configuration out for each file descriptor (same reaction)
                    reaction->reactor.emit<emit::Direct>(io_config);

                    // Return our handles
                    return std::make_tuple(port, cfd);
                }

                template <typename DSL>
                static inline Packet get(threading::Reaction& r) {
                    return UDP::get<DSL>(r);
                }
            };
        };
    }  // namespace word

    namespace trait {

        template <>
        struct is_transient<word::UDP::Packet> : public std::true_type {};

    }  // namespace trait
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_WORD_UDP_HPP
