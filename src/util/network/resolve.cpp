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

#include "resolve.hpp"

#include <cstdint>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <system_error>

#include "../platform.hpp"
#include "sock_t.hpp"

namespace NUClear {
namespace util {
    namespace network {

        NUClear::util::network::sock_t resolve(const std::string& address, const uint16_t& port) {
            addrinfo hints{};
            hints.ai_family   = AF_UNSPEC;  // don't care about IPv4 or IPv6
            hints.ai_socktype = AF_UNSPEC;  // don't care about TCP or UDP
            hints.ai_flags    = AI_ALL;     // Get all addresses even if we don't have an interface for them

            // Get our info on this address
            addrinfo* servinfo_ptr = nullptr;
            if (::getaddrinfo(address.c_str(), std::to_string(port).c_str(), &hints, &servinfo_ptr) != 0) {
                throw std::system_error(
                    network_errno,
                    std::system_category(),
                    "Failed to get address information for " + address + ":" + std::to_string(port));
            }

            // Check if we have any addresses to work with
            if (servinfo_ptr == nullptr) {
                throw std::runtime_error("Unable to find an address for " + address + ":" + std::to_string(port));
            }

            std::unique_ptr<addrinfo, void (*)(addrinfo*)> servinfo(servinfo_ptr,
                                                                    [](addrinfo* ptr) { ::freeaddrinfo(ptr); });

            // Empty sock_t struct
            NUClear::util::network::sock_t target{};

            // The list is actually a linked list of valid addresses
            // The address we choose is in the following priority, IPv4, IPv6, Other
            for (addrinfo* p = servinfo.get(); p != nullptr; p = p->ai_next) {

                // If we find an IPv4 address, prefer that
                if (servinfo->ai_family == AF_INET) {

                    // Set our target IPv4 address
                    std::memcpy(&target, servinfo->ai_addr, servinfo->ai_addrlen);

                    // We prefer IPv4 so use it and stop looking
                    return target;
                }

                // If we find an IPv6 address, hold the first one in case we don't find an IPv4 address
                if (servinfo->ai_family == AF_INET6) {

                    // // Set our target IPv6 address
                    std::memcpy(&target, servinfo->ai_addr, servinfo->ai_addrlen);
                }
            }

            // Found addresses, but none of them were IPv4 or IPv6
            if (target.sock.sa_family == AF_UNSPEC) {
                throw std::runtime_error("Unable to find an address for " + address + ":" + std::to_string(port));
            }

            return target;
        }

    }  // namespace network
}  // namespace util

}  // namespace NUClear
