/*
 * MIT License
 *
 * Copyright (c) 2015 NUClear Contributors
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

#ifndef NUCLEAR_DSL_WORD_UDP_HPP
#define NUCLEAR_DSL_WORD_UDP_HPP

#include <array>
#include <stdexcept>

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
         * This allows a reaction to be triggered based on UDP activity originating from external sources, or UDP
         * emissions within the system.
         *
         * @code on<UDP>(port) @endcode
         * When a connection is identified on the assigned port, the associated reaction will be triggered.
         * The request for a UDP based reaction can use a runtime argument to reference a specific port.
         * Note that the port reference cannot be changed during the systems execution phase.
         *
         * @code on<UDP>(port, bind_address) @endcode
         * The `bind_address` parameter can be used to specify which interface to bind on.
         * If `bind_address` is an empty string, the system will bind to any available interface.
         *
         * @code on<UDP>() @endcode
         * Should the port reference be omitted, then the system will bind to a currently unassigned port.
         *
         * @code on<UDP:Broadcast>(port)
         * on<UDP:Multicast>(multicast_address, port) @endcode
         * If needed, this trigger can also listen for UDP activity such as broadcast and multicast.
         *
         * These requests support both IPv4 and IPv6 addressing.
         *
         * @par Implements
         *  Bind
         */
        struct UDP : IO {
        private:
            /**
             * This structure is used to configure the UDP connection
             */
            struct ConnectOptions {
                /// The type of connection we are making
                enum class Type : uint8_t { UNICAST, BROADCAST, MULTICAST };
                /// The type of connection we are making
                Type type{};
                /// The address we are binding to or empty for any
                std::string bind_address;
                /// The port we are binding to or 0 for any
                in_port_t port = 0;
                /// The multicast address we are listening on or empty for any
                std::string target_address;
            };

            /**
             * This structure is used to return the result of a recvmsg call
             */
            struct RecvResult {
                /// If the packet is valid
                bool valid{false};
                /// The data that was received
                std::vector<uint8_t> payload;
                /// The local address that the packet was received on
                util::network::sock_t local{};
                /// The remote address that the packet was received from
                util::network::sock_t remote{};
            };

        public:
            struct Packet {
                Packet() = default;

                /// If the packet is valid (it contains data)
                bool valid{false};

                struct Target {
                    Target() = default;
                    Target(std::string address, const uint16_t& port) : address(std::move(address)), port(port) {}

                    /// The address of the target
                    std::string address;
                    /// The port of the target
                    uint16_t port{0};
                };

                /// The information about this packets destination
                Target local;
                /// The information about this packets source
                Target remote;

                /// The data to be sent in the packet
                std::vector<uint8_t> payload;

                /**
                 * Casts this packet to a boolean to check if it is valid
                 *
                 * @return true if the packet is valid
                 */
                operator bool() const {
                    return valid;
                }

                // We can cast ourselves to a reference type so long as
                // that reference type is plain old data
                template <typename T>
                operator std::enable_if_t<std::is_trivially_copyable<T>::value, const T&>() {
                    return *reinterpret_cast<const T*>(payload.data());
                }
            };

            template <typename DSL>
            static std::tuple<in_port_t, fd_t> connect(const std::shared_ptr<threading::Reaction>& reaction,
                                                       const ConnectOptions& options) {

                // Resolve the addresses
                util::network::sock_t bind_address{};
                util::network::sock_t multicast_target{};
                if (options.type == ConnectOptions::Type::MULTICAST) {
                    multicast_target = util::network::resolve(options.target_address, options.port);

                    // If there is no bind address, make sure we bind to an address of the same family
                    if (options.bind_address.empty()) {
                        bind_address = multicast_target;
                        switch (bind_address.sock.sa_family) {
                            case AF_INET: {
                                bind_address.ipv4.sin_port        = htons(options.port);
                                bind_address.ipv4.sin_addr.s_addr = htonl(INADDR_ANY);
                            } break;
                            case AF_INET6: {
                                bind_address.ipv6.sin6_port = htons(options.port);
                                bind_address.ipv6.sin6_addr = in6addr_any;
                            } break;
                            default: throw std::invalid_argument("Unknown socket family");
                        }
                    }
                    else {
                        bind_address = util::network::resolve(options.bind_address, options.port);
                        if (multicast_target.sock.sa_family != bind_address.sock.sa_family) {
                            throw std::invalid_argument("Multicast address family does not match bind address family");
                        }
                    }
                }
                else {
                    if (options.bind_address.empty()) {
                        bind_address.ipv4.sin_family      = AF_INET;
                        bind_address.ipv4.sin_port        = htons(options.port);
                        bind_address.ipv4.sin_addr.s_addr = htonl(INADDR_ANY);
                    }
                    else {
                        bind_address = util::network::resolve(options.bind_address, options.port);
                    }
                }

                // Make our socket
                util::FileDescriptor fd = ::socket(bind_address.sock.sa_family, SOCK_DGRAM, IPPROTO_UDP);
                if (!fd.valid()) {
                    throw std::system_error(network_errno, std::system_category(), "Unable to open the UDP socket");
                }

                // Include struct in the message "ancillary" control data
                int yes = 1;
                if (bind_address.sock.sa_family == AF_INET) {
                    if (::setsockopt(fd, IPPROTO_IP, IP_PKTINFO, reinterpret_cast<const char*>(&yes), sizeof(yes))
                        < 0) {
                        throw std::system_error(network_errno,
                                                std::system_category(),
                                                "Unable to flag the socket as getting ancillary data");
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
                                                "Unable to flag the socket as getting ancillary data");
                    }
                }

                // Broadcast and multicast reuse address and port
                if (options.type == ConnectOptions::Type::BROADCAST
                    || options.type == ConnectOptions::Type::MULTICAST) {

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
                if (options.type == ConnectOptions::Type::MULTICAST) {

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
                                                    "Unable to use the requested interface for multicast");
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
                                                    "Unable to use the requested interface for multicast");
                        }
                    }
                }

                // Get the port we ended up listening on
                socklen_t len = sizeof(bind_address);
                if (::getsockname(fd, &bind_address.sock, &len) == -1) {
                    throw std::system_error(network_errno,
                                            std::system_category(),
                                            "Unable to get the port from the UDP socket");
                }
                in_port_t port = 0;
                if (bind_address.sock.sa_family == AF_INET) {
                    port = ntohs(bind_address.ipv4.sin_port);
                }
                else if (bind_address.sock.sa_family == AF_INET6) {
                    port = ntohs(bind_address.ipv6.sin6_port);
                }
                else {
                    throw std::invalid_argument("Unknown socket family");
                }

                // Generate a reaction for the IO system that closes on death
                const fd_t cfd = fd.release();
                reaction->unbinders.push_back([cfd](const threading::Reaction&) { ::close(cfd); });
                IO::bind<DSL>(reaction, cfd, IO::READ | IO::CLOSE);

                // Return our handles and our bound port
                return std::make_tuple(port, cfd);
            }

            template <typename DSL>
            static RecvResult read(threading::ReactionTask& task) {
                // Get our file descriptor from the magic cache
                auto event = IO::get<DSL>(task);

                // If our get is being run without an fd (something else triggered)
                // Or if the event is not a read event then short circuit
                if (!event || (event.events & IO::READ) != IO::READ) {
                    return {};
                }

                // Allocate max size for a UDP packet
                std::vector<uint8_t> buffer(65535, 0);

                // Make some variables to hold our message header information
                std::array<char, 0x100> cmbuff = {0};
                util::network::sock_t remote{};
                iovec payload{};
                payload.iov_base = reinterpret_cast<char*>(buffer.data());
                payload.iov_len  = static_cast<decltype(payload.iov_len)>(buffer.size());

                // Make our message header to receive with
                msghdr mh{};
                mh.msg_name       = &remote.sock;
                mh.msg_namelen    = sizeof(util::network::sock_t);
                mh.msg_control    = cmbuff.data();
                mh.msg_controllen = cmbuff.size();
                mh.msg_iov        = &payload;
                mh.msg_iovlen     = 1;

                // Receive our message
                ssize_t received = recvmsg(event.fd, &mh, MSG_DONTWAIT);
                if (received < 0) {
                    return {};
                }

                buffer.resize(received);
                buffer.shrink_to_fit();

                // Load the socket we are listening on
                util::network::sock_t local{};
                socklen_t len = sizeof(util::network::sock_t);
                if (::getsockname(event.fd, &local.sock, &len) == -1) {
                    throw std::system_error(network_errno,
                                            std::system_category(),
                                            "Unable to get the port from the UDP socket");
                }

                // Iterate through control headers to get IP information
                for (cmsghdr* cmsg = CMSG_FIRSTHDR(&mh); cmsg != nullptr; cmsg = CMSG_NXTHDR(&mh, cmsg)) {
                    // If we find an ipv4 packet info header
                    if (local.sock.sa_family == AF_INET && cmsg->cmsg_level == IPPROTO_IP
                        && cmsg->cmsg_type == IP_PKTINFO) {

                        // Access the packet header information
                        const auto* pi = reinterpret_cast<in_pktinfo*>(reinterpret_cast<char*>(cmsg) + sizeof(*cmsg));
                        local.ipv4.sin_family = AF_INET;
                        local.ipv4.sin_addr   = pi->ipi_addr;

                        // We are done
                        break;
                    }

                    // If we find a ipv6 packet info header
                    if (local.sock.sa_family == AF_INET6 && cmsg->cmsg_level == IPPROTO_IPV6
                        && cmsg->cmsg_type == IPV6_PKTINFO) {

                        // Access the packet header information
                        const auto* pi = reinterpret_cast<in6_pktinfo*>(reinterpret_cast<char*>(cmsg) + sizeof(*cmsg));
                        local.ipv6.sin6_addr = pi->ipi6_addr;

                        // We are done
                        break;
                    }
                }

                return RecvResult{true, buffer, local, remote};
            }

            template <typename DSL>
            static std::tuple<in_port_t, fd_t> bind(const std::shared_ptr<threading::Reaction>& reaction,
                                                    const in_port_t& port           = 0,
                                                    const std::string& bind_address = "") {
                return connect<DSL>(reaction, ConnectOptions{ConnectOptions::Type::UNICAST, bind_address, port, ""});
            }

            template <typename DSL>
            static Packet get(threading::ReactionTask& task) {
                RecvResult result = read<DSL>(task);

                Packet p{};
                p.valid       = result.valid;
                p.payload     = std::move(result.payload);
                auto local_s  = result.local.address();
                auto remote_s = result.remote.address();
                p.local       = Packet::Target{local_s.first, local_s.second};
                p.remote      = Packet::Target{remote_s.first, remote_s.second};

                // Confirm that this packet was sent to one of our local addresses
                for (const auto& iface : util::network::get_interfaces()) {
                    if (iface.ip.sock.sa_family == result.local.sock.sa_family) {
                        // If the two are equal
                        if (iface.ip.sock.sa_family == AF_INET) {
                            if (iface.ip.ipv4.sin_addr.s_addr == result.local.ipv4.sin_addr.s_addr) {
                                return p;
                            }
                        }
                        else if (iface.ip.sock.sa_family == AF_INET6) {
                            if (std::memcmp(&iface.ip.ipv6.sin6_addr,
                                            &result.local.ipv6.sin6_addr,
                                            sizeof(result.local.ipv6.sin6_addr))
                                == 0) {
                                return p;
                            }
                        }
                    }
                }

                return {};
            }

            struct Broadcast : IO {

                template <typename DSL>
                static std::tuple<in_port_t, fd_t> bind(const std::shared_ptr<threading::Reaction>& reaction,
                                                        const in_port_t& port           = 0,
                                                        const std::string& bind_address = "") {
                    return UDP::connect<DSL>(reaction,
                                             ConnectOptions{ConnectOptions::Type::BROADCAST, bind_address, port, ""});
                }

                template <typename DSL>
                static Packet get(threading::ReactionTask& task) {
                    RecvResult result = read<DSL>(task);

                    // Broadcast is only IPv4
                    if (result.local.sock.sa_family == AF_INET) {

                        Packet p{};
                        p.valid       = result.valid;
                        p.payload     = std::move(result.payload);
                        auto local_s  = result.local.address();
                        auto remote_s = result.remote.address();
                        p.local       = Packet::Target{local_s.first, local_s.second};
                        p.remote      = Packet::Target{remote_s.first, remote_s.second};

                        // 255.255.255.255 is always a valid broadcast address
                        if (result.local.ipv4.sin_addr.s_addr == htonl(INADDR_BROADCAST)) {
                            return p;
                        }

                        // Confirm that this packet was sent to one of our broadcast addresses
                        for (const auto& iface : util::network::get_interfaces()) {
                            if (iface.broadcast.sock.sa_family == AF_INET) {
                                if (iface.flags.broadcast
                                    && iface.broadcast.ipv4.sin_addr.s_addr == result.local.ipv4.sin_addr.s_addr) {
                                    return p;
                                }
                            }
                        }
                    }

                    return {};
                }
            };

            struct Multicast : IO {

                template <typename DSL>
                static std::tuple<in_port_t, fd_t> bind(const std::shared_ptr<threading::Reaction>& reaction,
                                                        const std::string& multicast_group,
                                                        const in_port_t& port           = 0,
                                                        const std::string& bind_address = "") {
                    return UDP::connect<DSL>(
                        reaction,
                        ConnectOptions{ConnectOptions::Type::MULTICAST, bind_address, port, multicast_group});
                }

                template <typename DSL>
                static Packet get(threading::ReactionTask& task) {
                    RecvResult result = read<DSL>(task);

                    const auto& a = result.local;
                    const bool multicast =
                        (a.sock.sa_family == AF_INET && (ntohl(a.ipv4.sin_addr.s_addr) & 0xF0000000) == 0xE0000000)
                        || (a.sock.sa_family == AF_INET6 && a.ipv6.sin6_addr.s6_addr[0] == 0xFF);

                    // Only return multicast packets
                    if (multicast) {
                        Packet p{};
                        p.valid       = result.valid;
                        p.payload     = std::move(result.payload);
                        auto local_s  = result.local.address();
                        auto remote_s = result.remote.address();
                        p.local       = Packet::Target{local_s.first, local_s.second};
                        p.remote      = Packet::Target{remote_s.first, remote_s.second};
                        return p;
                    }

                    return {};
                }
            };
        };

    }  // namespace word

    namespace trait {

        template <>
        struct is_transient<word::UDP::Packet> : std::true_type {};

    }  // namespace trait
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_WORD_UDP_HPP
