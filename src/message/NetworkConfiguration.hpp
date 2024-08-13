/*
 * MIT License
 *
 * Copyright (c) 2013 NUClear Contributors
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

#ifndef NUCLEAR_MESSAGE_NETWORK_CONFIGURATION_HPP
#define NUCLEAR_MESSAGE_NETWORK_CONFIGURATION_HPP

#include <string>

namespace NUClear {
namespace message {

    struct NetworkConfiguration {

        NetworkConfiguration() = default;

        NetworkConfiguration(std::string name,
                             std::string address,
                             uint16_t port,
                             std::string bind_address = "",
                             uint16_t mtu             = 1500)
            : name(std::move(name))
            , announce_address(std::move(address))
            , announce_port(port)
            , bind_address(std::move(bind_address))
            , mtu(mtu) {}

        /// The name of this node when connecting to the NUClear network
        std::string name;
        /// The address to announce to the NUClear network
        std::string announce_address;
        /// The port to announce to the NUClear network
        uint16_t announce_port{0};
        /// The address of the interface to bind to when connecting to the NUClear network
        std::string bind_address;
        /// The maximum transmission unit for this node
        uint16_t mtu{1500};
    };

}  // namespace message
}  // namespace NUClear

#endif  // NUCLEAR_MESSAGE_NETWORK_CONFIGURATION_HPP
