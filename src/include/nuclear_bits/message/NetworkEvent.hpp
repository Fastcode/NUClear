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

#ifndef NUCLEAR_MESSAGE_NETWORKEVENT_HPP
#define NUCLEAR_MESSAGE_NETWORKEVENT_HPP

namespace NUClear {
    namespace message {

        struct NetworkJoin {
            NetworkJoin() : name(""), address(0), udpPort(0), tcpPort(0) {}

            std::string name;
            in_addr_t address;
            in_port_t udpPort;
            in_port_t tcpPort;
        };

        struct NetworkLeave {
            NetworkLeave() : name(""), address(0), udpPort(0), tcpPort(0) {}

            std::string name;
            in_addr_t address;
            in_port_t udpPort;
            in_port_t tcpPort;
        };

    }  // namespace message
 }  // namespace NUClear

#endif  // NUCLEAR_MESSAGE_NETWORKEVENT_HPP
