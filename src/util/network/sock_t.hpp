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
#include <cstdint>
#include <cstring>
#include <ostream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <tuple>

#include "../platform.hpp"

namespace NUClear {
namespace util {
    namespace network {

        /**
         * A struct representing a socket address, supporting both IPv4 and IPv6.
         * This struct provides a unified interface for handling socket addresses across different address families.
         */
        struct sock_t {
            /**
             * A union of socket address structures, allowing access to the address in different formats.
             * This union includes sockaddr_storage, sockaddr, sockaddr_in, and sockaddr_in6.
             */
            union {
                sockaddr_storage storage;  //< The storage for the socket address
                sockaddr sock;             //< The socket address
                sockaddr_in ipv4;          //< The IPv4 address
                sockaddr_in6 ipv6;         //< The IPv6 address
            };

            /**
             * Equality comparison operator for sock_t.
             *
             * @param a The first sock_t object
             * @param b The second sock_t object
             *
             * @return true if the addresses are equal, false otherwise.
             */
            friend bool operator==(const sock_t& a, const sock_t& b) {
                if ((a.sock.sa_family != AF_INET && a.sock.sa_family != AF_INET6)
                    || (b.sock.sa_family != AF_INET && b.sock.sa_family != AF_INET6)) {
                    throw std::system_error(EAFNOSUPPORT, std::system_category(), "Unsupported address family");
                }

                if (a.sock.sa_family != b.sock.sa_family) {
                    return false;
                }

                if (a.sock.sa_family == AF_INET) {
                    return a.ipv4.sin_port == b.ipv4.sin_port && a.ipv4.sin_addr.s_addr == b.ipv4.sin_addr.s_addr;
                }

                return a.ipv6.sin6_port == b.ipv6.sin6_port
                       && std::memcmp(&a.ipv6.sin6_addr.s6_addr, &b.ipv6.sin6_addr.s6_addr, sizeof(in6_addr)) == 0;
            }

            /**
             * Inequality comparison operator for sock_t.
             *
             * @param a The first sock_t object
             * @param b The second sock_t object
             *
             * @return true if the addresses are not equal, false otherwise
             */
            friend bool operator!=(const sock_t& a, const sock_t& b) {
                return !(a == b);
            }

            /**
             * Less-than comparison operator for sock_t.
             *
             * @param a The first sock_t object
             * @param b The second sock_t object
             *
             * @return true if a is less than b, false otherwise
             */
            friend bool operator<(const sock_t& a, const sock_t& b) {
                if ((a.sock.sa_family != AF_INET && a.sock.sa_family != AF_INET6)
                    || (b.sock.sa_family != AF_INET && b.sock.sa_family != AF_INET6)) {
                    throw std::system_error(EAFNOSUPPORT, std::system_category(), "Unsupported address family");
                }

                if (a.sock.sa_family != b.sock.sa_family) {
                    return a.sock.sa_family < b.sock.sa_family;
                }
                if (a.sock.sa_family == AF_INET) {
                    return std::forward_as_tuple(ntohl(a.ipv4.sin_addr.s_addr), ntohs(a.ipv4.sin_port))
                           < std::forward_as_tuple(ntohl(b.ipv4.sin_addr.s_addr), ntohs(b.ipv4.sin_port));
                }

                auto cmp = std::memcmp(a.ipv6.sin6_addr.s6_addr, b.ipv6.sin6_addr.s6_addr, sizeof(a.ipv6.sin6_addr));
                if (cmp != 0) {
                    return cmp < 0;
                }

                return ntohs(a.ipv6.sin6_port) < ntohs(b.ipv6.sin6_port);
            }

            /**
             * Returns the size of the socket address structure.
             *
             * @return The size of the socket address structure
             *
             * @throws std::system_error if the address family is unsupported
             */
            socklen_t size() const;

            /**
             * Resolves the socket address to a hostname and port.
             *
             * @param numeric If true, returns the numeric IP address instead of the hostname
             *
             * @return A pair containing the hostname (or numeric IP) and the port
             *
             * @throws std::system_error if the address cannot be resolved
             */
            std::pair<std::string, in_port_t> address(bool numeric = false) const;

            /**
             * Output stream operator for sock_t.
             * Outputs the address in the format "{host}:{port}"
             *
             * @param os The output stream to write to
             * @param addr The socket address to output
             * @return The output stream
             */
            friend std::ostream& operator<<(std::ostream& os, const sock_t& addr) {
                auto addr_pair = addr.address(true);
                return os << addr_pair.first << ":" << addr_pair.second;
            }
        };

    }  // namespace network
}  // namespace util
}  // namespace NUClear

#endif  // NUCLEAR_UTIL_NETWORK_SOCK_T_HPP
