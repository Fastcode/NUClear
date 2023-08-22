/*
 * Copyright (C) 2013      Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
 *               2014-2023 Trent Houliston <trent@houliston.me>
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

#include "../platform.hpp"
#include "get_interfaces.hpp"

namespace NUClear {
namespace util {
    namespace network {

        int if_number_from_address(const sockaddr_in6& ipv6) {

            // If the socket is referring to the any address, return 0 to use the default interface
            if (std::all_of(ipv6.sin6_addr.s6_addr, ipv6.sin6_addr.s6_addr + 16, [](unsigned char i) {
                    return i == 0;
                })) {
                return 0;
            }

            // Find the correct interface to join on (the one that has our bind address)
            for (auto& iface : get_interfaces()) {
                // iface must be, ipv6, multicast, and have the same address as our bind address
                if (iface.ip.sock.sa_family == AF_INET6 && iface.flags.multicast
                    && ::memcmp(iface.ip.ipv6.sin6_addr.s6_addr, ipv6.sin6_addr.s6_addr, sizeof(in6_addr)) == 0) {

                    // Get the interface for this
                    return ::if_nametoindex(iface.name.c_str());
                }
            }

            // If we get here then we couldn't find an interface
            throw std::runtime_error("Could not find interface for address");
        }

    }  // namespace network
}  // namespace util
}  // namespace NUClear
