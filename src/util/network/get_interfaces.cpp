/*
 * Copyright (C) 2013-2016 Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
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

#include "nuclear_bits/util/network/get_interfaces.hpp"

// Include platform specific details
#include "nuclear_bits/util/platform.hpp"

#ifndef _WIN32
#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>
#include <sys/socket.h>
#endif

#include <algorithm>
#include <cstring>
#include <system_error>

namespace NUClear {
namespace util {
    namespace network {

// Windows version
#ifdef _WIN32
        std::vector<Interface> get_interfaces() {

            std::vector<Interface> ifaces;

            DWORD size = 0;

            // Do an initial call to work out how much space we need
            GetIpAddrTable(nullptr, &size, false);

            // Allocate enough space for the real call
            std::vector<MIB_IPADDRTABLE> table((size / sizeof(MIB_IPADDRTABLE)) + 1);

            // Do the real call
            GetIpAddrTable(table.data(), &size, false);

            for (size_t i = 0; i < table.front().dwNumEntries; ++i) {

                // Storage for the information for this address
                MIB_IF_ROW2 data;
                memset(&data, 0, sizeof(data));
                data.InterfaceIndex = table[i].table->dwIndex;
                if (GetIfEntry2(&data) != NO_ERROR) {
                    continue;
                };

                Interface iface;
                auto n                = std::wstring(data.Alias);
                iface.name            = std::string(n.begin(), n.end());
                iface.ip              = htonl(table[i].table->dwAddr);
                iface.netmask         = htonl(table[i].table->dwMask);
                iface.broadcast       = iface.ip | ~iface.netmask;
                iface.flags.broadcast = data.AccessType == NET_IF_ACCESS_BROADCAST;
                iface.flags.loopback  = data.AccessType == NET_IF_ACCESS_LOOPBACK;
                iface.flags.multicast = data.AccessType == NET_IF_ACCESS_BROADCAST;
                iface.flags.pointtopoint =
                    data.AccessType == NET_IF_ACCESS_LOOPBACK || data.AccessType == NET_IF_ACCESS_POINT_TO_POINT;

                ifaces.push_back(iface);
            }

            return ifaces;
        }

// Everyone else
#else
        std::vector<Interface> get_interfaces() {

            std::vector<Interface> ifaces;

            addrinfo hints;
            std::memset(&hints, 0, sizeof(hints));
            hints.ai_family = AF_INET;

            // Query our interfaces
            ifaddrs* addrs;
            if (getifaddrs(&addrs) < 0) {
                throw std::system_error(
                    network_errno, std::system_category(), "Unable to query the interfaces on the platform");
            }

            // Loop through our interfaces
            for (ifaddrs* cursor = addrs; cursor != nullptr; cursor = cursor->ifa_next) {

                // We only care about ipv4 addresses (one day this will need to change)
                Interface iface;

                // Clear!
                std::memset(&iface, 0, sizeof(iface));

                iface.name = cursor->ifa_name;

                // Copy across our various addresses
                switch (cursor->ifa_addr->sa_family) {
                    case AF_INET: std::memcpy(&iface.ip, cursor->ifa_addr, sizeof(sockaddr_in)); break;

                    case AF_INET6: std::memcpy(&iface.ip, cursor->ifa_addr, sizeof(sockaddr_in6)); break;
                }
                
                if (cursor->ifa_netmask) {
                switch (cursor->ifa_addr->sa_family) {
                    case AF_INET: std::memcpy(&iface.netmask, cursor->ifa_netmask, sizeof(sockaddr_in)); break;

                    case AF_INET6: std::memcpy(&iface.netmask, cursor->ifa_netmask, sizeof(sockaddr_in6)); break;
                }}

                if (cursor->ifa_dstaddr) {
                switch (cursor->ifa_addr->sa_family) {
                    case AF_INET: std::memcpy(&iface.broadcast, cursor->ifa_dstaddr, sizeof(sockaddr_in)); break;

                    case AF_INET6: std::memcpy(&iface.broadcast, cursor->ifa_dstaddr, sizeof(sockaddr_in6)); break;
                }}

                iface.flags.broadcast    = (cursor->ifa_flags & IFF_BROADCAST) != 0;
                iface.flags.loopback     = (cursor->ifa_flags & IFF_LOOPBACK) != 0;
                iface.flags.pointtopoint = (cursor->ifa_flags & IFF_POINTOPOINT) != 0;
                iface.flags.multicast    = (cursor->ifa_flags & IFF_MULTICAST) != 0;

                ifaces.push_back(iface);
            }

            // Free memory
            freeifaddrs(addrs);

            return ifaces;
        }
#endif
    }
}
}
