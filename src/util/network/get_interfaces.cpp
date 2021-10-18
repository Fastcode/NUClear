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

#include "get_interfaces.hpp"

#include <algorithm>
#include <cstring>
#include <system_error>

#include "../platform.hpp"

namespace NUClear {
namespace util {
    namespace network {

// Windows version
#ifdef _WIN32
        std::vector<Interface> get_interfaces() {

            std::vector<Interface> ifaces;

            // First call with null to work out how much memory we need
            ULONG size = 0;
            if (GetAdaptersAddresses(AF_UNSPEC, 0, nullptr, nullptr, &size) != ERROR_BUFFER_OVERFLOW) {
                throw std::runtime_error("Unable to query the list of network interfaces");
            }
            else {
                // Allocate some memory now and call again
                PIP_ADAPTER_ADDRESSES addrs = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(std::malloc(size));
                auto rv                     = GetAdaptersAddresses(AF_UNSPEC,
                                               GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST
                                                   | GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_FRIENDLY_NAME,
                                               nullptr,
                                               addrs,
                                               &size);
                if (rv != ERROR_SUCCESS) {
                    free(addrs);
                    throw std::runtime_error("Unable to query the list of network interfaces");
                }

                for (PIP_ADAPTER_ADDRESSES addr = addrs; addr != nullptr; addr = addr->Next) {
                    // Skip adapters that are not up and able to process packets (e.g. they're disconnected, disabled,
                    // etc)
                    if (addr->OperStatus != IfOperStatusUp) { continue; }

                    for (auto uaddr = addr->FirstUnicastAddress; uaddr != nullptr; uaddr = uaddr->Next) {

                        Interface iface;
                        std::memset(&iface, 0, sizeof(iface));

                        iface.name = addr->AdapterName;

                        // Copy across the IP address
                        std::memcpy(&iface.ip, uaddr->Address.lpSockaddr, uaddr->Address.iSockaddrLength);

                        switch (iface.ip.sock.sa_family) {
                            case AF_INET: {
                                // IPv4 address
                                auto& ipv4      = *reinterpret_cast<sockaddr_in*>(&iface.ip);
                                auto& netmask   = *reinterpret_cast<sockaddr_in*>(&iface.netmask);
                                auto& broadcast = *reinterpret_cast<sockaddr_in*>(&iface.broadcast);

                                // Fill in the netmask
                                netmask.sin_family = AF_INET;
                                ConvertLengthToIpv4Mask(uaddr->OnLinkPrefixLength, &netmask.sin_addr.s_addr);

                                // Fill in the broadcast
                                broadcast.sin_family      = AF_INET;
                                broadcast.sin_addr.s_addr = (ipv4.sin_addr.s_addr | (~netmask.sin_addr.s_addr));

                                // Loopback if the ip address starts with 127
                                iface.flags.loopback = (ipv4.sin_addr.s_addr & htonl(0x7F000000)) == htonl(0x7F000000);
                                // Point to point if the netmask is all 1s
                                iface.flags.pointtopoint = (netmask.sin_addr.s_addr & 0xFFFFFFFF) == 0xFFFFFFFF;
                                // Broadcast if not point to point
                                iface.flags.broadcast = !iface.flags.pointtopoint;
                                // Multicast if broadcast
                                iface.flags.multicast = iface.flags.broadcast;

                            } break;
                            case AF_INET6: {
                                // IPv6 address
                                auto& ipv6    = *reinterpret_cast<sockaddr_in6*>(&iface.ip);
                                auto& netmask = *reinterpret_cast<sockaddr_in6*>(&iface.netmask);

                                // Fill in the netmask
                                netmask.sin6_family = AF_INET6;

                                // Fill the netmask from the bits
                                for (int i = 0; i < uaddr->OnLinkPrefixLength; ++i) {
                                    netmask.sin6_addr.s6_addr[i / 8] |= 1 << (7 - (i % 8));
                                }

                                // IPv6 doesn't have broadcast

                                // Loopback if the ip address is ::1
                                iface.flags.loopback = ipv6.sin6_addr.s6_addr[15] == 0x01;
                                for (int i = 0; i < 15; ++i) {
                                    iface.flags.loopback &= ipv6.sin6_addr.s6_addr[i] == 0;
                                }

                                // Point to point if the netmask is all 1s
                                iface.flags.pointtopoint = true;
                                for (int i = 0; i < 16; ++i) {
                                    iface.flags.pointtopoint &= netmask.sin6_addr.s6_addr[i] == 0xFF;
                                }

                                // No broadcast on IPv6
                                iface.flags.broadcast = false;
                                // IPv6 is always multicast
                                iface.flags.multicast = true;

                            } break;
                        }

                        ifaces.push_back(iface);
                    }
                }

                std::free(addrs);
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
            if (::getifaddrs(&addrs) < 0) {
                throw std::system_error(
                    network_errno, std::system_category(), "Unable to query the interfaces on the platform");
            }

            // Loop through our interfaces
            for (ifaddrs* cursor = addrs; cursor != nullptr; cursor = cursor->ifa_next) {

                // Sometimes we find an interface with no IP (like a CAN bus) this is not what we're after
                if (cursor->ifa_addr) {

                    Interface iface;
                    iface.name = cursor->ifa_name;

                    // Copy across our various addresses
                    switch (cursor->ifa_addr->sa_family) {
                        case AF_INET: std::memcpy(&iface.ip, cursor->ifa_addr, sizeof(sockaddr_in)); break;

                        case AF_INET6: std::memcpy(&iface.ip, cursor->ifa_addr, sizeof(sockaddr_in6)); break;
                    }

                    if (cursor->ifa_netmask != nullptr) {
                        switch (cursor->ifa_addr->sa_family) {
                            case AF_INET: std::memcpy(&iface.netmask, cursor->ifa_netmask, sizeof(sockaddr_in)); break;

                            case AF_INET6:
                                std::memcpy(&iface.netmask, cursor->ifa_netmask, sizeof(sockaddr_in6));
                                break;
                        }
                    }
                    else {
                        std::memset(&iface.netmask, 0, sizeof(iface.netmask));
                    }

                    if (cursor->ifa_dstaddr != nullptr) {
                        switch (cursor->ifa_addr->sa_family) {
                            case AF_INET:
                                std::memcpy(&iface.broadcast, cursor->ifa_dstaddr, sizeof(sockaddr_in));
                                break;

                            case AF_INET6:
                                std::memcpy(&iface.broadcast, cursor->ifa_dstaddr, sizeof(sockaddr_in6));
                                break;
                        }
                    }
                    else {
                        std::memset(&iface.broadcast, 0, sizeof(iface.broadcast));
                    }

                    iface.flags.broadcast    = (cursor->ifa_flags & IFF_BROADCAST) != 0;
                    iface.flags.loopback     = (cursor->ifa_flags & IFF_LOOPBACK) != 0;
                    iface.flags.pointtopoint = (cursor->ifa_flags & IFF_POINTOPOINT) != 0;
                    iface.flags.multicast    = (cursor->ifa_flags & IFF_MULTICAST) != 0;

                    ifaces.push_back(iface);
                }
            }

            // Free memory
            ::freeifaddrs(addrs);

            return ifaces;
        }
#endif
    }  // namespace network
}  // namespace util
}  // namespace NUClear
