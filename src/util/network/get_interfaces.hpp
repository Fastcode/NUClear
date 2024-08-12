/*
 * MIT License
 *
 * Copyright (c) 2015 NUClear Contributors
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

#ifndef NUCLEAR_UTIL_NETWORK_GET_INTERFACES_HPP
#define NUCLEAR_UTIL_NETWORK_GET_INTERFACES_HPP

#include <string>
#include <vector>

#include "sock_t.hpp"

namespace NUClear {
namespace util {
    namespace network {

        /**
         * A structure that contains information about a network interface.
         */
        struct Interface {
            /// The name of the interface
            std::string name;

            /// The address that is bound to the interface
            sock_t ip{};
            /// The netmask of the interface
            sock_t netmask{};
            /// The broadcast address of the interface or point to point address
            sock_t broadcast{};

            struct Flags {
                /// True if the interface is a broadcast interface
                bool broadcast{false};
                /// True if the interface is a loopback interface
                bool loopback{false};
                /// True if the interface is a point to point interface
                bool pointtopoint{false};
                /// True if the interface is a multicast interface
                bool multicast{false};
            };
            /// The flags that are set on the interface
            Flags flags;
        };

        /**
         * Gets a list of all the network interfaces on the system with the addresses they are bound to.
         *
         * @return A list of all the interfaces on the system
         */
        std::vector<Interface> get_interfaces();

    }  // namespace network
}  // namespace util
}  // namespace NUClear

#endif  // NUCLEAR_UTIL_NETWORK_GET_INTERFACES_HPP
