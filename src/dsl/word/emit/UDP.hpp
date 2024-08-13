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

#ifndef NUCLEAR_DSL_WORD_EMIT_UDP_HPP
#define NUCLEAR_DSL_WORD_EMIT_UDP_HPP

#include <stdexcept>

#include "../../../PowerPlant.hpp"
#include "../../../util/FileDescriptor.hpp"
#include "../../../util/network/if_number_from_address.hpp"
#include "../../../util/platform.hpp"
#include "../../../util/serialise/Serialise.hpp"
#include "../../store/DataStore.hpp"
#include "../../store/TypeCallbackStore.hpp"

namespace NUClear {
namespace dsl {
    namespace word {
        namespace emit {

            /**
             * Emits data as a UDP packet over the network.
             *
             * @code emit<Scope::UDP>(data, to_addr, to_port); @endcode
             * Emissions under this scope are useful for communicating with other systems using UDP.
             * The target of the packet can be can be a unicast, broadcast or multicast address, specified as a string.
             * Additionally the address and port on the local machine can be specified using a string and port.
             *
             * @attention
             *  Anything emitted over the UDP network must be serialisable.
             *
             * @param data      the data to emit
             * @param to_addr   a string specifying the address to send this packet to
             * @param to_port   the port to send this packet to in host endian
             * @param from_addr Optional. The address to send this from or "" to automatically choose an address.
             * @param from_port Optional. The port to send this from in host endian or 0 to automatically choose a port.
             * @tparam DataType the datatype of the object to emit
             */
            template <typename DataType>
            struct UDP {

                static void emit(const PowerPlant& /*powerplant*/,
                                 std::shared_ptr<DataType> data,
                                 const std::string& to_addr,
                                 in_port_t to_port,
                                 const std::string& from_addr = "",
                                 in_port_t from_port          = 0) {

                    // Resolve our addresses
                    const util::network::sock_t remote = util::network::resolve(to_addr, to_port);
                    const bool multicast =
                        remote.sock.sa_family == AF_INET    ? ((remote.ipv4.sin_addr.s_addr >> 28) == 0xE)
                        : remote.sock.sa_family == AF_INET6 ? ((remote.ipv6.sin6_addr.s6_addr[0] & 0xFF) == 0xFF)
                                                            : false;

                    // If we are not provided a from address, use any from address
                    util::network::sock_t local{};
                    if (from_addr.empty()) {
                        // By default have the settings of local match remote (except address and port)
                        local = remote;
                        switch (local.sock.sa_family) {
                            case AF_INET: {
                                local.ipv4.sin_port        = htons(from_port);
                                local.ipv4.sin_addr.s_addr = htonl(INADDR_ANY);
                            } break;
                            case AF_INET6: {
                                local.ipv6.sin6_port = htons(from_port);
                                local.ipv6.sin6_addr = IN6ADDR_ANY_INIT;
                            } break;
                            default: throw std::invalid_argument("Unknown socket family");
                        }
                    }
                    else {
                        local = util::network::resolve(from_addr, from_port);
                        if (local.sock.sa_family != remote.sock.sa_family) {
                            throw std::invalid_argument("to and from addresses are not the same family");
                        }
                    }

                    // Open a socket to send the datagram from
                    util::FileDescriptor fd = ::socket(local.sock.sa_family, SOCK_DGRAM, IPPROTO_UDP);
                    if (!fd.valid()) {
                        throw std::system_error(network_errno, std::system_category(), "Unable to open the UDP socket");
                    }

                    // If we are using multicast and we have a specific from_addr we need to tell the system to use the
                    // correct interface
                    if (multicast && !from_addr.empty()) {
                        if (local.sock.sa_family == AF_INET) {
                            // Set our transmission interface for the multicast socket
                            if (::setsockopt(fd,
                                             IPPROTO_IP,
                                             IP_MULTICAST_IF,
                                             reinterpret_cast<const char*>(&local.ipv4.sin_addr),
                                             sizeof(local.ipv4.sin_addr))
                                < 0) {
                                throw std::system_error(network_errno,
                                                        std::system_category(),
                                                        "Unable to use the requested interface for multicast");
                            }
                        }
                        else if (local.sock.sa_family == AF_INET6) {
                            // Set our transmission interface for the multicast socket
                            auto if_number = util::network::if_number_from_address(local.ipv6);
                            if (::setsockopt(fd,
                                             IPPROTO_IPV6,
                                             IPV6_MULTICAST_IF,
                                             reinterpret_cast<const char*>(&if_number),
                                             sizeof(if_number))
                                < 0) {
                                throw std::system_error(network_errno,
                                                        std::system_category(),
                                                        "Unable to use the requested interface for multicast");
                            }
                        }
                    }

                    // Bind a local port if requested
                    if (!from_addr.empty() || from_port != 0) {
                        if (::bind(fd, &local.sock, local.size()) != 0) {
                            throw std::system_error(network_errno,
                                                    std::system_category(),
                                                    "Unable to bind the UDP socket to the port");
                        }
                    }

                    // Assume that if the user is sending a broadcast they want to enable broadcasting
                    int yes = 1;
                    if (::setsockopt(fd, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<const char*>(&yes), sizeof(yes))
                        < 0) {
                        throw std::system_error(network_errno,
                                                std::system_category(),
                                                "Unable to enable broadcasting on this socket");
                    }

                    // Serialise to our payload
                    std::vector<uint8_t> payload = util::serialise::Serialise<DataType>::serialise(*data);

                    // Try to send our payload
                    if (::sendto(fd,
                                 reinterpret_cast<const char*>(payload.data()),
                                 static_cast<socklen_t>(payload.size()),
                                 0,
                                 &remote.sock,
                                 remote.size())
                        < 0) {
                        throw std::system_error(network_errno,
                                                std::system_category(),
                                                "Unable to send the UDP message");
                    }
                }
            };

        }  // namespace emit
    }  // namespace word
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_WORD_EMIT_UDP_HPP
