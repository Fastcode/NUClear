/*
 * Copyright (C) 2013 Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
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

#ifndef NUCLEAR_DSL_WORD_IO_H
#define NUCLEAR_DSL_WORD_IO_H

#ifdef _WIN32
    #include "nuclear_bits/util/windows_includes.hpp"
#else
    #include <poll.h>
#endif

#include "nuclear_bits/dsl/operation/Unbind.hpp"
#include "nuclear_bits/dsl/word/emit/Direct.hpp"
#include "nuclear_bits/dsl/word/Single.hpp"
#include "nuclear_bits/dsl/store/ThreadStore.hpp"
#include "nuclear_bits/dsl/trait/is_transient.hpp"
#include "nuclear_bits/util/generate_reaction.hpp"
#include "nuclear_bits/util/platform.hpp"

namespace NUClear {
    namespace dsl {
        namespace word {

            struct IOConfiguration {
                fd_t fd;
                int events;
                std::shared_ptr<threading::Reaction> reaction;
            };

            // IO is implicitly single
            struct IO : public Single {

                // On windows we use different wait events
                #ifdef _WIN32
                enum EventType : short {
                    READ = FD_READ | FD_OOB | FD_ACCEPT,
                    WRITE = FD_WRITE,
                    CLOSE = FD_CLOSE,
                    FAIL = 0
                };
                #else
                enum EventType : short {
                    READ = POLLIN,
                    WRITE = POLLOUT,
                    CLOSE = POLLHUP,
                    FAIL = POLLNVAL | POLLERR
                };
                #endif

                struct Event {
                    fd_t fd;
                    int events;

                    operator bool() const {
                        return fd != 0;
                    }
                };

                using ThreadEventStore = dsl::store::ThreadStore<Event>;

                template <typename DSL, typename TFunc>
                static inline threading::ReactionHandle bind(Reactor& reactor, const std::string& label, TFunc&& callback, fd_t fd, int watchSet) {

                    auto reaction = util::generate_reaction<DSL, IO>(reactor, label, std::forward<TFunc>(callback));

                    auto ioConfig = std::make_unique<IOConfiguration>(IOConfiguration {
                        fd,
                        watchSet,
                        std::move(reaction)
                    });

                    threading::ReactionHandle handle(ioConfig->reaction);

                    // Send our configuration out
                    reactor.powerplant.emit<emit::Direct>(ioConfig);

                    // Return our handles
                    return handle;
                }

                template <typename DSL>
                static inline Event get(threading::Reaction&) {

                    // If our thread store has a value
                    if(ThreadEventStore::value) {
                        return *ThreadEventStore::value;
                    }
                    // Otherwise return an empty event
                    else {
                        return Event { 0, 0 };
                    }
                }
            };
        }

        namespace trait {
            template <>
            struct is_transient<word::IO::Event> : public std::true_type {};
        }
    }
}

#endif  // NUCLEAR_DSL_WORD_IO_H
