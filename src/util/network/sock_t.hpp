/*
 * MIT License
 *
 * Copyright (c) 2017 NUClear Contributors
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

#ifndef NUCLEAR_UTIL_NETWORK_SOCK_T_HPP
#define NUCLEAR_UTIL_NETWORK_SOCK_T_HPP

#include <array>
#include <stdexcept>

#include "../platform.hpp"

namespace NUClear {
namespace util {
    namespace network {

        struct sock_t {
            union {
                sockaddr_storage storage;
                sockaddr sock;
                sockaddr_in ipv4;
                sockaddr_in6 ipv6;
            };

            socklen_t size() const {
                switch (sock.sa_family) {
                    case AF_INET: return sizeof(sockaddr_in);
                    case AF_INET6: return sizeof(sockaddr_in6);
                    default:
                        throw std::runtime_error("Cannot get size for socket address family "
                                                 + std::to_string(sock.sa_family));
                }
            }

            std::pair<std::string, in_port_t> address() const {
                std::array<char, std::max(INET_ADDRSTRLEN, INET6_ADDRSTRLEN)> c = {0};
                switch (sock.sa_family) {
                    case AF_INET:
                        return std::make_pair(::inet_ntop(sock.sa_family, &ipv4.sin_addr, c.data(), c.size()),
                                              ntohs(ipv4.sin_port));
                    case AF_INET6:
                        return std::make_pair(::inet_ntop(sock.sa_family, &ipv6.sin6_addr, c.data(), c.size()),
                                              ntohs(ipv6.sin6_port));
                    default:
                        throw std::runtime_error("Cannot get address for socket address family "
                                                 + std::to_string(sock.sa_family));
                }
            }
        };

    }  // namespace network
}  // namespace util
}  // namespace NUClear

#endif  // NUCLEAR_UTIL_NETWORK_SOCK_T_HPP
