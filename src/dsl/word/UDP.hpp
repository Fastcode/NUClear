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

#include <array>

#include "../../PowerPlant.hpp"
#include "../../threading/Reaction.hpp"
#include "../../util/FileDescriptor.hpp"
#include "../../util/network/get_interfaces.hpp"
#include "../../util/network/if_number_from_address.hpp"
#include "../../util/network/resolve.hpp"
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
        struct UDP : public IO {
        private:
            struct Target {
                std::string bind_address{};
                in_port_t port = 0;
                std::string target_address{};
                enum Type { UNICAST, BROADCAST, MULTICAST } type{};
            };

        public:
            struct Packet {
                Packet() = default;

                /// @brief If the packet is valid (it contains data)
                bool valid{false};

                struct Target {
                    Target() = default;
                    Target(std::string address, const uint16_t& port) : address(std::move(address)), port(port) {}

                    /// The address of the target
                    std::string address{};
                    /// The port of the target
                    uint16_t port{0};
                };

                /// @brief The information about this packets destination
                Target local;
                /// @brief The information about this packets source
                Target remote;

                /// @brief The data to be sent in the packet
                std::vector<char> payload{};

                /**
                 * @brief Casts this packet to a boolean to check if it is valid
                 *
                 * @return true if the packet is valid
                 */
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
            static inline std::tuple<in_port_t, fd_t> connect(const std::shared_ptr<threading::Reaction>& reaction,
                                                              const Target& target) {

                // Resolve the addresses
                util::network::sock_t bind_address{};
                util::network::sock_t multicast_target{};
                if (target.type == Target::MULTICAST) {
                    multicast_target = util::network::resolve(target.target_address, target.port);
                    if (target.bind_address.empty()) {
                        bind_address = multicast_target;
                        switch (bind_address.sock.sa_family) {
                            case AF_INET: {
                                bind_address.ipv4.sin_port        = htons(target.port);
                                bind_address.ipv4.sin_addr.s_addr = htonl(INADDR_ANY);
                            } break;
                            case AF_INET6: {
                                bind_address.ipv6.sin6_port = htons(target.port);
                                bind_address.ipv6.sin6_addr = in6addr_any;
                            } break;
                            default: throw std::runtime_error("Unknown socket family");
                        }
                    }
                    else {
                        bind_address = util::network::resolve(target.bind_address, target.port);
                        if (multicast_target.sock.sa_family != bind_address.sock.sa_family) {
                            throw std::runtime_error("Multicast address family does not match bind address family");
                        }
                    }
                }
                else {
                    if (target.bind_address.empty()) {
                        bind_address.ipv4.sin_family      = AF_INET;
                        bind_address.ipv4.sin_port        = htons(target.port);
                        bind_address.ipv4.sin_addr.s_addr = htonl(INADDR_ANY);
                    }
                    else {
                        bind_address = util::network::resolve(target.bind_address, target.port);
                    }
                }

                // Make our socket
                util::FileDescriptor fd = ::socket(bind_address.sock.sa_family, SOCK_DGRAM, IPPROTO_UDP);
                if (!fd.valid()) {
                    throw std::system_error(network_errno, std::system_category(), "Unable to open the UDP socket");
                }

                int yes = 1;
                // Include struct in the message "ancillary" control data
                if (bind_address.sock.sa_family == AF_INET) {
                    if (::setsockopt(fd, IPPROTO_IP, IP_PKTINFO, reinterpret_cast<const char*>(&yes), sizeof(yes))
                        < 0) {
                        throw std::system_error(network_errno,
                                                std::system_category(),
                                                "We were unable to flag the socket as getting ancillary data");
                    }
                }
                else if (bind_address.sock.sa_family == AF_INET6) {
                    if (::setsockopt(fd,
                                     IPPROTO_IPV6,
                                     IPV6_RECVPKTINFO,
                                     reinterpret_cast<const char*>(&yes),
                                     sizeof(yes))
                        < 0) {
                    throw std::system_error(network_errno,
                                            std::system_category(),
                                            "We were unable to flag the socket as getting ancillary data");
                }
                }

                // Broadcast and multicast reuse address and port
                if (target.type == Target::BROADCAST || target.type == Target::MULTICAST) {

                    if (::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char*>(&yes), sizeof(yes)) < 0) {
                        throw std::system_error(network_errno,
                                                std::system_category(),
                                                "Unable to reuse address on the socket");
                    }

// If SO_REUSEPORT is available set it too
#ifdef SO_REUSEPORT
                    if (::setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, reinterpret_cast<char*>(&yes), sizeof(yes)) < 0) {
                        throw std::system_error(network_errno,
                                                std::system_category(),
                                                "Unable to reuse port on the socket");
                    }
#endif

                    // We enable SO_BROADCAST since sometimes we need to send broadcast packets
                    if (::setsockopt(fd, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<char*>(&yes), sizeof(yes)) < 0) {
                        throw std::system_error(network_errno,
                                                std::system_category(),
                                                "Unable to set broadcast on the socket");
                    }
                }

                // Bind to the address
                if (::bind(fd, &bind_address.sock, bind_address.size()) != 0) {
                    throw std::system_error(network_errno, std::system_category(), "Unable to bind the UDP socket");
                }

                // If we have a multicast address, then we need to join the multicast groups
                if (target.type == Target::MULTICAST) {

                    // Our multicast join request will depend on protocol version
                    if (multicast_target.sock.sa_family == AF_INET) {

                        // Set the multicast address we are listening on and bind address
                        ip_mreq mreq{};
                        mreq.imr_multiaddr = multicast_target.ipv4.sin_addr;
                        mreq.imr_interface = bind_address.ipv4.sin_addr;

                        // Join our multicast group
                        if (::setsockopt(fd,
                                         IPPROTO_IP,
                                         IP_ADD_MEMBERSHIP,
                                         reinterpret_cast<char*>(&mreq),
                                         sizeof(ip_mreq))
                            < 0) {
                            throw std::system_error(network_errno,
                                                    std::system_category(),
                                                    "There was an error while attempting to join the multicast group");
                        }

                        // Set our transmission interface for the multicast socket
                        if (::setsockopt(fd,
                                         IPPROTO_IP,
                                         IP_MULTICAST_IF,
                                         reinterpret_cast<const char*>(&bind_address.ipv4.sin_addr),
                                         sizeof(bind_address.ipv4.sin_addr))
                            < 0) {
                            throw std::system_error(network_errno,
                                                    std::system_category(),
                                                    "We were unable to use the requested interface for multicast");
                        }
                    }
                    else if (multicast_target.sock.sa_family == AF_INET6) {

                        // Set the multicast address we are listening on
                        ipv6_mreq mreq{};
                        mreq.ipv6mr_multiaddr = multicast_target.ipv6.sin6_addr;
                        mreq.ipv6mr_interface = util::network::if_number_from_address(bind_address.ipv6);

                        // Join our multicast group
                        if (::setsockopt(fd,
                                         IPPROTO_IPV6,
                                         IPV6_JOIN_GROUP,
                                         reinterpret_cast<char*>(&mreq),
                                         sizeof(ipv6_mreq))
                            < 0) {
                            throw std::system_error(network_errno,
                                                    std::system_category(),
                                                    "There was an error while attempting to join the multicast group");
                        }

                        // Set our transmission interface for the multicast socket
                        if (::setsockopt(fd,
                                         IPPROTO_IPV6,
                                         IPV6_MULTICAST_IF,
                                         reinterpret_cast<const char*>(&mreq.ipv6mr_interface),
                                         sizeof(mreq.ipv6mr_interface))
                            < 0) {
                            throw std::system_error(network_errno,
                                                    std::system_category(),
                                                    "We were unable to use the requested interface for multicast");
                        }
                    }
                }

                // Get the port we ended up listening on
                socklen_t len = sizeof(sockaddr_in);
                if (::getsockname(fd, reinterpret_cast<sockaddr*>(&bind_address), &len) == -1) {
                    throw std::system_error(network_errno,
                                            std::system_category(),
                                            "We were unable to get the port from the UDP socket");
                }
                in_port_t port;
                if (bind_address.sock.sa_family == AF_INET) {
                    port = ntohs(bind_address.ipv4.sin_port);
                }
                else if (bind_address.sock.sa_family == AF_INET6) {
                    port = ntohs(bind_address.ipv6.sin6_port);
                }
                else {
                    throw std::runtime_error("Unknown socket family");
                }

                // Generate a reaction for the IO system that closes on death
                const fd_t cfd = fd.release();
                reaction->unbinders.push_back([cfd](const threading::Reaction&) { ::close(cfd); });
                IO::bind<DSL>(reaction, cfd, IO::READ | IO::CLOSE);

                // Return our handles and our bound port
                return std::make_tuple(port, cfd);
                return std::make_tuple(in_port_t(0), fd_t(0));
            }

            template <typename DSL>
            static inline std::tuple<in_port_t, fd_t> bind(const std::shared_ptr<threading::Reaction>& reaction,
                                                           in_port_t port           = 0,
                                                           std::string bind_address = "") {
                Target t;
                t.type         = Target::UNICAST;
                t.bind_address = std::move(bind_address);
                t.port         = port;
                return connect<DSL>(reaction, t);
            }

            template <typename DSL>
            static inline Packet get(threading::Reaction& reaction) {

                // Get our file descriptor from the magic cache
                auto event = IO::get<DSL>(reaction);

                // If our get is being run without an fd (something else triggered) then short circuit
                if (!event) {
                    return Packet{};
                }

                // Allocate max size for a UDP packet
                Packet p{};
                p.payload.resize(65535);

                // Make some variables to hold our message header information
                std::array<char, 0x100> cmbuff = {0};
                util::network::sock_t remote{};
                iovec payload{};
                payload.iov_base = p.payload.data();
                payload.iov_len  = static_cast<decltype(payload.iov_len)>(p.payload.size());

                // Make our message header to receive with
                msghdr mh{};
                mh.msg_name       = &remote.sock;
                mh.msg_namelen    = sizeof(util::network::sock_t);
                mh.msg_control    = cmbuff.data();
                mh.msg_controllen = cmbuff.size();
                mh.msg_iov        = &payload;
                mh.msg_iovlen     = 1;

                // Receive our message
                ssize_t received = recvmsg(event.fd, &mh, 0);

                // Load the socket we are listening on
                util::network::sock_t local{};
                socklen_t len = sizeof(util::network::sock_t);
                if (::getsockname(event.fd, &local.sock, &len) == -1) {
                    throw std::system_error(network_errno,
                                            std::system_category(),
                                            "We were unable to get the port from the UDP socket");
                }

                // Iterate through control headers to get IP information
                for (cmsghdr* cmsg = CMSG_FIRSTHDR(&mh); cmsg != nullptr; cmsg = CMSG_NXTHDR(&mh, cmsg)) {
                    // ignore the control headers that don't match what we want
                    if (local.sock.sa_family == AF_INET && cmsg->cmsg_level == IPPROTO_IP
                        && cmsg->cmsg_type == IP_PKTINFO) {

                        // Access the packet header information
                        auto* pi = reinterpret_cast<in_pktinfo*>(reinterpret_cast<char*>(cmsg) + sizeof(*cmsg));
                        local.ipv4.sin_family = AF_INET;
                        local.ipv4.sin_addr   = pi->ipi_addr;

                        // We are done
                        break;
                    }
                    else if (local.sock.sa_family == AF_INET6 && cmsg->cmsg_level == IPPROTO_IPV6
                             && cmsg->cmsg_type == IPV6_PKTINFO) {

                        // Access the packet header information
                        auto* pi = reinterpret_cast<in6_pktinfo*>(reinterpret_cast<char*>(cmsg) + sizeof(*cmsg));
                        local.ipv6.sin6_addr = pi->ipi6_addr;

                        // We are done
                        break;
                    }
                }


                // if no error
                if (received > 0) {
                    p.valid                                   = true;
                    std::tie(p.local.address, p.local.port)   = local.address();
                    std::tie(p.remote.address, p.remote.port) = remote.address();
                    p.payload.resize(size_t(received));
                    p.payload.shrink_to_fit();
                }

                return p;
            }

            struct Broadcast : public IO {

                template <typename DSL>
                static inline std::tuple<in_port_t, fd_t> bind(const std::shared_ptr<threading::Reaction>& reaction,
                                                               in_port_t port           = 0,
                                                               std::string bind_address = "") {
                    Target t;
                    t.type         = Target::BROADCAST;
                    t.bind_address = std::move(bind_address);
                    t.port         = port;
                    return UDP::connect<DSL>(reaction, t);
                }

                template <typename DSL>
                static inline Packet get(threading::Reaction& reaction) {
                    return UDP::get<DSL>(reaction);
                }
            };

            struct Multicast : public IO {

                template <typename DSL>
                static inline std::tuple<in_port_t, fd_t> bind(const std::shared_ptr<threading::Reaction>& reaction,
                                                               const std::string& multicast_group,
                                                               in_port_t port                  = 0,
                                                               const std::string& bind_address = "") {
                    Target t;
                    t.type           = Target::MULTICAST;
                    t.bind_address   = bind_address;
                    t.target_address = multicast_group;
                    t.port           = port;
                    return UDP::connect<DSL>(reaction, t);
                }

                template <typename DSL>
                static inline Packet get(threading::Reaction& reaction) {
                    return UDP::get<DSL>(reaction);
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
