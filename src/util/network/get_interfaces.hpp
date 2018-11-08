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

#ifndef NUCLEAR_UTIL_NETWORK_GET_INTERFACES_HPP
#define NUCLEAR_UTIL_NETWORK_GET_INTERFACES_HPP

#include <string>
#include <vector>

#include "sock_t.hpp"

namespace NUClear {
namespace util {
    namespace network {

        struct Interface {
            Interface() : name(""), ip(), netmask(), broadcast(), flags() {}

            std::string name;

            sock_t ip;
            sock_t netmask;
            sock_t broadcast;

            struct Flags {
                Flags() : broadcast(false), loopback(false), pointtopoint(false), multicast(false) {}

                bool broadcast;
                bool loopback;
                bool pointtopoint;
                bool multicast;
            } flags;
        };

        std::vector<Interface> get_interfaces();

    }  // namespace network
}  // namespace util
}  // namespace NUClear

#endif  // NUCLEAR_UTIL_NETWORK_GET_INTERFACES_HPP
