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

#ifndef NUCLEAR_MESSAGE_NETWORKCONFIGURATION_HPP
#define NUCLEAR_MESSAGE_NETWORKCONFIGURATION_HPP

namespace NUClear {
namespace message {

    struct NetworkConfiguration {

        NetworkConfiguration() : name(""), announce_address(""), announce_port(0), mtu(1500) {}

        NetworkConfiguration(const std::string& name, const std::string& address, uint16_t port, uint16_t mtu = 1500)
            : name(name), announce_address(address), announce_port(port), mtu(mtu) {}

        std::string name;
        std::string announce_address;
        uint16_t announce_port;
        uint16_t mtu;
    };

}  // namespace message
}  // namespace NUClear

#endif  // NUCLEAR_MESSAGE_NETWORKCONFIGURATION_HPP
