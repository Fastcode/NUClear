/*
 * MIT License
 *
 * Copyright (c) 2023 NUClear Contributors
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

#include "if_number_from_address.hpp"

#include <algorithm>
#include <cstring>
#include <iterator>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../platform.hpp"
#include "get_interfaces.hpp"
#include "sock_t.hpp"

namespace NUClear {
namespace util {
    namespace network {

        unsigned int if_number_from_address(const sockaddr_in6& ipv6) {

            // If the socket is referring to the any address, return 0 to use the default interface
            if (std::all_of(std::begin(ipv6.sin6_addr.s6_addr), std::end(ipv6.sin6_addr.s6_addr), [](unsigned char i) {
                    return i == 0;
                })) {
                return 0;
            }

            // Find the correct interface to join on (the one that has our bind address)
            for (const auto& iface : get_interfaces()) {
                // iface must be, ipv6, and have the same address as our bind address
                if (iface.ip.sock.sa_family == AF_INET6
                    && ::memcmp(iface.ip.ipv6.sin6_addr.s6_addr, ipv6.sin6_addr.s6_addr, sizeof(in6_addr)) == 0) {

                    // Get the interface for this
                    return ::if_nametoindex(iface.name.c_str());
                }
            }

            // If we get here then we couldn't find an interface
            sock_t s{};
            s.ipv6 = ipv6;
            auto a = s.address();
            throw std::runtime_error("Could not find interface for address " + a.first + " (is it up?)");
        }

    }  // namespace network
}  // namespace util
}  // namespace NUClear
