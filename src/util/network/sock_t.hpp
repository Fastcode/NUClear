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

#include <algorithm>
#include <array>
#include <ostream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <tuple>

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

            /// Equality comparison operator
            friend bool operator==(const sock_t& a, const sock_t& b) {
                if (a.sock.sa_family != b.sock.sa_family) {
                    return false;
                }
                if (a.sock.sa_family == AF_INET) {
                    return a.ipv4.sin_port == b.ipv4.sin_port
                           && a.ipv4.sin_addr.s_addr == b.ipv4.sin_addr.s_addr;
                }
                if (a.sock.sa_family == AF_INET6) {
                    return a.ipv6.sin6_port == b.ipv6.sin6_port
                           && std::equal(std::begin(a.ipv6.sin6_addr.s6_addr),
                                         std::end(a.ipv6.sin6_addr.s6_addr),
                                         std::begin(b.ipv6.sin6_addr.s6_addr));
                }
                return false;
            }

            /// Inequality comparison operator
            friend bool operator!=(const sock_t& a, const sock_t& b) {
                return !(a == b);
            }

            /// Less-than comparison for use as map key
            friend bool operator<(const sock_t& a, const sock_t& b) {
                if (a.sock.sa_family != b.sock.sa_family) {
                    return a.sock.sa_family < b.sock.sa_family;
                }
                if (a.sock.sa_family == AF_INET) {
                    return std::forward_as_tuple(ntohl(a.ipv4.sin_addr.s_addr), ntohs(a.ipv4.sin_port))
                           < std::forward_as_tuple(ntohl(b.ipv4.sin_addr.s_addr), ntohs(b.ipv4.sin_port));
                }
                if (a.sock.sa_family == AF_INET6) {
                    const bool addr_less = std::lexicographical_compare(std::begin(a.ipv6.sin6_addr.s6_addr),
                                                                        std::end(a.ipv6.sin6_addr.s6_addr),
                                                                        std::begin(b.ipv6.sin6_addr.s6_addr),
                                                                        std::end(b.ipv6.sin6_addr.s6_addr));
                    if (addr_less) {
                        return true;
                    }
                    const bool addr_greater = std::lexicographical_compare(std::begin(b.ipv6.sin6_addr.s6_addr),
                                                                           std::end(b.ipv6.sin6_addr.s6_addr),
                                                                           std::begin(a.ipv6.sin6_addr.s6_addr),
                                                                           std::end(a.ipv6.sin6_addr.s6_addr));
                    if (addr_greater) {
                        return false;
                    }
                    return ntohs(a.ipv6.sin6_port) < ntohs(b.ipv6.sin6_port);
                }
                return false;
            }

            /// Stream output operator
            friend std::ostream& operator<<(std::ostream& os, const sock_t& addr) {
                auto addr_pair = addr.address(true);
                return os << addr_pair.first << ":" << addr_pair.second;
            }

            socklen_t size() const {
                switch (sock.sa_family) {
                    case AF_INET: return sizeof(sockaddr_in);
                    case AF_INET6: return sizeof(sockaddr_in6);
                    default:
                        throw std::runtime_error("Cannot get size for socket address family "
                                                 + std::to_string(sock.sa_family));
                }
            }

            std::pair<std::string, in_port_t> address(bool numeric_host = false) const {
                std::array<char, NI_MAXHOST> host{};
                std::array<char, NI_MAXSERV> service{};
                const int result = ::getnameinfo(reinterpret_cast<const sockaddr*>(&storage),
                                                 size(),
                                                 host.data(),
                                                 static_cast<socklen_t>(host.size()),
                                                 service.data(),
                                                 static_cast<socklen_t>(service.size()),
                                                 NI_NUMERICSERV | (numeric_host ? NI_NUMERICHOST : 0));
                if (result != 0) {
                    throw std::system_error(
                        network_errno,
                        std::system_category(),
                        "Cannot get address for socket address family " + std::to_string(sock.sa_family));
                }
                return std::make_pair(std::string(host.data()), static_cast<in_port_t>(std::stoi(service.data())));
            }
        };

    }  // namespace network
}  // namespace util
}  // namespace NUClear

#endif  // NUCLEAR_UTIL_NETWORK_SOCK_T_HPP
