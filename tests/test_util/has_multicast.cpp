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

#include "util/network/get_interfaces.hpp"
#include "util/platform.hpp"

namespace test_util {

bool has_ipv4_multicast() {
    // See if any interface has multicast ipv4
    auto ifaces = NUClear::util::network::get_interfaces();
    return std::any_of(ifaces.begin(), ifaces.end(), [](const auto& iface) {
        return iface.ip.sock.sa_family == AF_INET && iface.flags.multicast;
    });
}

bool has_ipv6_multicast() {
    // See if any interface has multicast ipv6
    auto ifaces = NUClear::util::network::get_interfaces();
    return std::any_of(ifaces.begin(), ifaces.end(), [](const auto& iface) {
        return iface.ip.sock.sa_family == AF_INET6 && iface.flags.multicast;
    });
}

}  // namespace test_util
