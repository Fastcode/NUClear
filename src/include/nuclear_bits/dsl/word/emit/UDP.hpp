/*
 * Copyright (C) 2013 Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
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

#ifndef NUCLEAR_DSL_WORD_EMIT_UDP_H
#define NUCLEAR_DSL_WORD_EMIT_UDP_H

#ifdef _WIN32
    #include "nuclear_bits/util/windows_includes.hpp"
#else
    #include <unistd.h>
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <net/if.h>
    #include <cstring>
#endif

#include "nuclear_bits/PowerPlant.hpp"
#include "nuclear_bits/util/FileDescriptor.hpp"
#include "nuclear_bits/util/serialise/Serialise.hpp"
#include "nuclear_bits/dsl/store/DataStore.hpp"
#include "nuclear_bits/dsl/store/TypeCallbackStore.hpp"

namespace NUClear {
    namespace dsl {
        namespace word {
            namespace emit {

                template <typename TData>
                struct UDP {

                    static inline void emit(PowerPlant&, std::shared_ptr<TData> input, uint32_t toAddr, uint16_t toPort, uint32_t fromAddr, uint16_t fromPort) {

                        sockaddr_in src;
                        sockaddr_in target;
                        memset(&src, 0, sizeof(sockaddr_in));
                        memset(&target, 0, sizeof(sockaddr_in));

                        // Get socket addresses for our source and target
                        src.sin_family = AF_INET;
                        src.sin_addr.s_addr = htonl(fromAddr);
                        src.sin_port = htons(fromPort);

                        target.sin_family = AF_INET;
                        target.sin_addr.s_addr = htonl(toAddr);
                        target.sin_port = htons(toPort);

                        // Work out if we are sending to a multicast address
                        bool multicast = ((toAddr >> 28) == 14);

                        // Open a socket to send the datagram from
                        util::FileDescriptor fd = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
                        if(fd < 0) {
                            throw std::system_error(network_errno, std::system_category(), "We were unable to open the UDP socket");
                        }

                        // If we need to, bind to a port on our end
                        if(fromAddr != 0 || fromPort != 0) {
                            if(::bind(fd, reinterpret_cast<sockaddr*>(&src), sizeof(sockaddr))) {
                                throw std::system_error(network_errno, std::system_category(), "We were unable to bind the UDP socket to the port");
                            }
                        }

                        // If we are using multicast and we have a specific fromAddr we need to tell the system to use it
                        if(multicast && fromAddr != 0) {
                            // Set our transmission interface for the multicast socket
                            if(setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF, reinterpret_cast<const char*>(&src.sin_addr), sizeof(src.sin_addr)) < 0) {
                                throw std::system_error(network_errno, std::system_category(), "We were unable to use the requested interface for multicast");
                            }
                        }

                        // This isn't the greatest code, but lets assume our users don't send broadcasts they don't mean to...
                        int yes = true;
                        if(setsockopt(fd, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<const char*>(&yes), sizeof(yes)) < 0) {
                            throw std::system_error(network_errno, std::system_category(), "We were unable to enable broadcasting on this socket");
                        }

                        // Serialise the data
                        std::vector<char> data = util::serialise::Serialise<TData>::serialise(*input);

                        // Try to send our data
                        if(::sendto(fd, data.data(), data.size(), 0, reinterpret_cast<sockaddr*>(&target), sizeof(sockaddr_in)) < 0) {
                            throw std::system_error(network_errno, std::system_category(), "We were unable to send the UDP message");
                        }
                    }

                    // String ip addresses
                    static inline void emit(PowerPlant& pp, std::shared_ptr<TData> data, std::string toAddr, uint16_t toPort, std::string fromAddr, uint16_t fromPort) {
                        
						in_addr addr;
                        
                        inet_pton(AF_INET, toAddr.c_str(), &addr);
                        uint32_t to = ntohl(addr.s_addr);
                        
                        inet_pton(AF_INET, fromAddr.c_str(), &addr);
                        uint32_t from = ntohl(addr.s_addr);

                        emit(pp, data, to, toPort, from, fromPort);

                    }

                    static inline void emit(PowerPlant& pp, std::shared_ptr<TData> data, std::string toAddr, uint16_t toPort, uint32_t fromAddr, uint16_t fromPort) {

                        in_addr addr;
                        
                        inet_pton(AF_INET, toAddr.c_str(), &addr);
                        uint32_t to = ntohl(addr.s_addr);
                        
                        emit(pp, data, to, toPort, fromAddr, fromPort);
                    }

                    static inline void emit(PowerPlant& pp, std::shared_ptr<TData> data, uint32_t toAddr, uint16_t toPort, std::string fromAddr, uint16_t fromPort) {
                        
						in_addr addr;
                        
                        inet_pton(AF_INET, fromAddr.c_str(), &addr);
                        uint32_t from = ntohl(addr.s_addr);
                        
                        emit(pp, data, toAddr, toPort, from, fromPort);
                    }

                    // No from address
                    static inline void emit(PowerPlant& pp, std::shared_ptr<TData> data, uint32_t toAddr, uint16_t toPort) {
                        emit(pp, data, toAddr, toPort, INADDR_ANY, 0);
                    }

                    static inline void emit(PowerPlant& pp, std::shared_ptr<TData> data, std::string toAddr, uint16_t toPort) {

						in_addr addr;

						inet_pton(AF_INET, toAddr.c_str(), &addr);
						uint32_t to = ntohl(addr.s_addr);

                        emit(pp, data, to, toPort, INADDR_ANY, 0);
                    }
                };
            }
        }
    }
}

#endif
