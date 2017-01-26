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

#ifndef NUCLEAR_EXTENSION_IOCONTROLLER_WINDOWS_HPP
#define NUCLEAR_EXTENSION_IOCONTROLLER_WINDOWS_HPP

#include "nuclear"
#include "nuclear_bits/dsl/word/IO.hpp"

#include "nuclear_bits/util/windows_includes.hpp"

namespace NUClear {
namespace extension {

    class IOController : public Reactor {
    public:
        explicit IOController(std::unique_ptr<NUClear::Environment> environment);
        ~IOController();

    private:
        struct Event {
            SOCKET fd;
            std::shared_ptr<threading::Reaction> reaction;
            int events;
        };

        WSAEVENT notifier;

        bool shutdown = false;
        std::mutex reaction_mutex;
        std::map<WSAEVENT, Event> reactions;
        std::vector<WSAEVENT> fds;
    };

}  // namespace extension
}  // namespace NUClear

#endif  // NUCLEAR_EXTENSION_IOCONTROLLER_WINDOWS_HPP
