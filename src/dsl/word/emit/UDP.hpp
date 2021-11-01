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

#ifndef NUCLEAR_DSL_WORD_EMIT_UDP_HPP
#define NUCLEAR_DSL_WORD_EMIT_UDP_HPP

#include "../../../PowerPlant.hpp"
#include "../../../util/FileDescriptor.hpp"
#include "../../../util/platform.hpp"
#include "../../../util/serialise/Serialise.hpp"
#include "../../store/DataStore.hpp"
#include "../../store/TypeCallbackStore.hpp"

namespace NUClear {
namespace dsl {
    namespace word {
        namespace emit {

            /**
             * @brief
             *  Emits data as a UDP packet over the network.
             *
             * @details
             *  @code emit<Scope::UDP>(data, to_addr, to_port); @endcode
             *  Emissions under this scope are useful for communicating with third parties. The target of the packet
             *  can be can be a unicast, broadcast or multicast address, specified as either a host endian int, or as a
             *  string. Additionally the address and port on the local machine can be specified using a string or host
             *  endian int.
             *
             * @attention
             *  Anything emitted over the UDP network must be serialisable.
             *
             * @param data      the data to emit
             * @param to_addr   a string or host endian integer specifying the ip to send the packet to
             * @param to_port   the port to send this packet to in host endian
             * @param from_addr Optional.  A string or host endian integer specifying the local ip to send the packet
             *                  from.  Defaults to INADDR_ANY.
             * @param from_port Optional.  The port to send this from to in host endian or 0 to automatically choose a
             *                  port. Defaults to 0.
             * @tparam DataType the datatype of the object to emit
             */
            template <typename DataType>
            struct UDP {

                static inline void emit(PowerPlant&,
                                        std::shared_ptr<DataType> data,
                                        in_addr_t to_addr,
                                        in_port_t to_port,
                                        in_addr_t from_addr,
                                        in_port_t from_port) {

                    sockaddr_in src;
                    sockaddr_in target;
                    memset(&src, 0, sizeof(sockaddr_in));
                    memset(&target, 0, sizeof(sockaddr_in));

                    // Get socket addresses for our source and target
                    src.sin_family      = AF_INET;
                    src.sin_addr.s_addr = htonl(from_addr);
                    src.sin_port        = htons(from_port);

                    target.sin_family      = AF_INET;
                    target.sin_addr.s_addr = htonl(to_addr);
                    target.sin_port        = htons(to_port);

                    // Work out if we are sending to a multicast address
                    bool multicast = ((to_addr >> 28) == 14);

                    // Open a socket to send the datagram from
                    util::FileDescriptor fd = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
                    if (fd < 0) {
                        throw std::system_error(
                            network_errno, std::system_category(), "We were unable to open the UDP socket");
                    }

                    // If we need to, bind to a port on our end
                    if (from_addr != 0 || from_port != 0) {
                        if (::bind(fd, reinterpret_cast<sockaddr*>(&src), sizeof(sockaddr))) {
                            throw std::system_error(network_errno,
                                                    std::system_category(),
                                                    "We were unable to bind the UDP socket to the port");
                        }
                    }

                    // If we are using multicast and we have a specific from_addr we need to tell the system to use it
                    if (multicast && from_addr != 0) {
                        // Set our transmission interface for the multicast socket
                        if (setsockopt(fd,
                                       IPPROTO_IP,
                                       IP_MULTICAST_IF,
                                       reinterpret_cast<const char*>(&src.sin_addr),
                                       sizeof(src.sin_addr))
                            < 0) {
                            throw std::system_error(network_errno,
                                                    std::system_category(),
                                                    "We were unable to use the requested interface for multicast");
                        }
                    }

                    // This isn't the greatest code, but lets assume our users don't send broadcasts they don't mean
                    // to...
                    int yes = true;
                    if (setsockopt(fd, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<const char*>(&yes), sizeof(yes))
                        < 0) {
                        throw std::system_error(network_errno,
                                                std::system_category(),
                                                "We were unable to enable broadcasting on this socket");
                    }

                    // Serialise to our payload
                    std::vector<char> payload = util::serialise::Serialise<DataType>::serialise(*data);

                    // Try to send our payload
                    if (::sendto(fd,
                                 payload.data(),
                                 static_cast<socklen_t>(payload.size()),
                                 0,
                                 reinterpret_cast<sockaddr*>(&target),
                                 sizeof(sockaddr_in))
                        < 0) {
                        throw std::system_error(
                            network_errno, std::system_category(), "We were unable to send the UDP message");
                    }
                }

                // String ip addresses
                static inline void emit(PowerPlant& pp,
                                        std::shared_ptr<DataType> data,
                                        std::string to_addr,
                                        in_port_t to_port,
                                        std::string from_addr,
                                        in_port_t from_port) {

                    in_addr addr;

                    inet_pton(AF_INET, to_addr.c_str(), &addr);
                    in_addr_t to = ntohl(addr.s_addr);

                    inet_pton(AF_INET, from_addr.c_str(), &addr);
                    in_addr_t from = ntohl(addr.s_addr);

                    emit(pp, data, to, to_port, from, from_port);
                }

                static inline void emit(PowerPlant& pp,
                                        std::shared_ptr<DataType> data,
                                        std::string to_addr,
                                        in_port_t to_port,
                                        in_addr_t from_addr,
                                        in_port_t from_port) {

                    in_addr addr;

                    inet_pton(AF_INET, to_addr.c_str(), &addr);
                    in_addr_t to = ntohl(addr.s_addr);

                    emit(pp, data, to, to_port, from_addr, from_port);
                }

                static inline void emit(PowerPlant& pp,
                                        std::shared_ptr<DataType> data,
                                        in_addr_t to_addr,
                                        in_port_t to_port,
                                        std::string from_addr,
                                        in_port_t from_port) {

                    in_addr addr;

                    inet_pton(AF_INET, from_addr.c_str(), &addr);
                    in_addr_t from = ntohl(addr.s_addr);

                    emit(pp, data, to_addr, to_port, from, from_port);
                }

                // No from address
                static inline void emit(PowerPlant& pp,
                                        std::shared_ptr<DataType> data,
                                        in_addr_t to_addr,
                                        in_port_t to_port) {
                    emit(pp, data, to_addr, to_port, INADDR_ANY, in_port_t(0));
                }

                static inline void emit(PowerPlant& pp,
                                        std::shared_ptr<DataType> data,
                                        std::string to_addr,
                                        in_port_t to_port) {

                    in_addr addr;

                    inet_pton(AF_INET, to_addr.c_str(), &addr);
                    in_addr_t to = ntohl(addr.s_addr);

                    emit(pp, data, to, to_port, INADDR_ANY, in_port_t(0));
                }
            };

        }  // namespace emit
    }      // namespace word
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_WORD_EMIT_UDP_HPP
