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

#include "has_multicast.hpp"

#include <algorithm>
#include <cstdint>
#include <string>
#include <system_error>

#include "util/FileDescriptor.hpp"
#include "util/network/get_interfaces.hpp"
#include "util/network/resolve.hpp"
#include "util/platform.hpp"

namespace test_util {
namespace {

/// Multicast addresses used by tests/tests/dsl/UDP.cpp.
constexpr uint16_t IPV4_MULTICAST_PROBE_PORT = 40003;
constexpr uint16_t IPV6_MULTICAST_PROBE_PORT = 40004;
const std::string IPV4_MULTICAST_ADDRESS     = "230.12.3.22";
const std::string IPV6_MULTICAST_ADDRESS     = "ff02::230:12:3:22";

bool can_send_udp_datagram(const std::string& to_addr,
                           const uint16_t to_port,
                           const std::string& bind_addr = "") {
    try {
        const NUClear::util::network::sock_t remote = NUClear::util::network::resolve(to_addr, to_port);
        NUClear::util::FileDescriptor fd           = ::socket(remote.sock.sa_family, SOCK_DGRAM, IPPROTO_UDP);
        if (!fd.valid()) {
            return false;
        }

        const int yes = 1;
        if (::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&yes), sizeof(yes)) < 0) {
            return false;
        }

        if (!bind_addr.empty()) {
            const NUClear::util::network::sock_t local = NUClear::util::network::resolve(bind_addr, 0);
            if (local.sock.sa_family != remote.sock.sa_family) {
                return false;
            }
            if (::bind(fd, &local.sock, local.size()) != 0) {
                return false;
            }
        }

        const char payload = 0;
        if (::sendto(fd, &payload, 1, 0, &remote.sock, remote.size()) < 0) {
            return false;
        }
        return true;
    }
    catch (const std::exception&) {
        return false;
    }
}

}  // namespace

bool has_ipv4_multicast() {
    const auto ifaces = NUClear::util::network::get_interfaces();
    const bool iface_multicast =
        std::any_of(ifaces.begin(), ifaces.end(), [](const auto& iface) {
            return iface.ip.sock.sa_family == AF_INET && iface.flags.multicast;
        });
    if (!iface_multicast) {
        return false;
    }
    return can_send_udp_datagram(IPV4_MULTICAST_ADDRESS, IPV4_MULTICAST_PROBE_PORT);
}

bool has_ipv6_multicast() {
    const auto ifaces = NUClear::util::network::get_interfaces();
    const bool iface_multicast =
        std::any_of(ifaces.begin(), ifaces.end(), [](const auto& iface) {
            return iface.ip.sock.sa_family == AF_INET6 && iface.flags.multicast;
        });
    if (!iface_multicast) {
        return false;
    }
#ifdef __APPLE__
    // Match UDP.cpp: bind to ::1 so sends succeed when there is no default IPv6 multicast route.
    return can_send_udp_datagram(IPV6_MULTICAST_ADDRESS, IPV6_MULTICAST_PROBE_PORT, "::1");
#else
    return can_send_udp_datagram(IPV6_MULTICAST_ADDRESS, IPV6_MULTICAST_PROBE_PORT);
#endif
}

}  // namespace test_util
