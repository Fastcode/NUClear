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

#ifndef NUCLEAR_UTIL_GET_NETWORK_INTERFACES_H
#define NUCLEAR_UTIL_GET_NETWORK_INTERFACES_H

#include <tuple>

#include <cstring>
#include <unistd.h>
#include <ifaddrs.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netdb.h>

namespace NUClear {
    namespace util {
        namespace network {

            struct Interface {
                std::string name;

                uint32_t ip;
                uint32_t netmask;
                uint32_t broadcast;

                struct {
                    bool broadcast;
                    bool loopback;
                    bool pointtopoint;
                    bool multicast;
                } flags;
            };

            inline std::vector<Interface> get_interfaces() {

                std::vector<Interface> ifaces;

                // Query our interfaces
                ifaddrs* addrs;
                if(getifaddrs(&addrs) < 0) {
                    throw std::system_error(errno, std::system_category(), "Unable to query the interfaces on the platform");
                }

                // Loop through our interfaces
                for(ifaddrs* cursor = addrs; cursor != nullptr; cursor = cursor->ifa_next) {

                    // We only care about ipv4 addresses (one day this will need to change)
                    if(cursor->ifa_addr->sa_family == AF_INET) {
                        Interface iface;

                        iface.name      = cursor->ifa_name;
                        iface.ip        = ntohl(reinterpret_cast<sockaddr_in*>(cursor->ifa_addr)->sin_addr.s_addr);
                        iface.netmask   = ntohl(reinterpret_cast<sockaddr_in*>(cursor->ifa_netmask)->sin_addr.s_addr);
                        iface.broadcast = ntohl(reinterpret_cast<sockaddr_in*>(cursor->ifa_dstaddr)->sin_addr.s_addr);

                        iface.flags.broadcast    = (cursor->ifa_flags & IFF_BROADCAST) != 0;
                        iface.flags.loopback     = (cursor->ifa_flags & IFF_LOOPBACK) != 0;
                        iface.flags.pointtopoint = (cursor->ifa_flags & IFF_POINTOPOINT) != 0;
                        iface.flags.multicast    = (cursor->ifa_flags & IFF_MULTICAST) != 0;

                        ifaces.push_back(iface);
                    }
                }

                // Remove duplicates from ifaces
                ifaces.erase(std::unique(std::begin(ifaces), std::end(ifaces), [] (const Interface& a, const Interface& b) {
                    return a.name == b.name;
                }), std::end(ifaces));

                return ifaces;
            }
        }
    }
}

#endif
