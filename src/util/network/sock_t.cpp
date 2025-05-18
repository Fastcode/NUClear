/*
 * MIT License
 *
 * Copyright (c) 2025 NUClear Contributors
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
#include "sock_t.hpp"

#include <netdb.h>

#include <cstdlib>

namespace NUClear {
namespace util {
    namespace network {

        socklen_t sock_t::size() const {
            switch (sock.sa_family) {
                case AF_INET: return sizeof(sockaddr_in);
                case AF_INET6: return sizeof(sockaddr_in6);
                default: throw std::system_error(EAFNOSUPPORT, std::system_category(), "Unsupported address family");
            }
        }

        std::pair<std::string, in_port_t> sock_t::address(bool numeric) const {
            std::array<char, NI_MAXHOST> host{};
            std::array<char, NI_MAXSERV> service{};

            if (::getnameinfo(&sock,
                              size(),
                              host.data(),
                              static_cast<socklen_t>(host.size()),
                              service.data(),
                              static_cast<socklen_t>(service.size()),
                              NI_NUMERICSERV | (numeric ? NI_NUMERICHOST : 0))
                != 0) {
                throw std::system_error(
                    network_errno,
                    std::system_category(),
                    "Cannot get address for socket address family " + std::to_string(sock.sa_family));
            }

            return {host.data(), static_cast<in_port_t>(std::stoi(service.data()))};
        }

    }  // namespace network
}  // namespace util
}  // namespace NUClear
