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

#ifndef NUCLEAR_DSL_WORD_IO_HPP
#define NUCLEAR_DSL_WORD_IO_HPP

#include "../../id.hpp"
#include "../../threading/Reaction.hpp"
#include "../../util/platform.hpp"
#include "../operation/Unbind.hpp"
#include "../store/ThreadStore.hpp"
#include "../trait/is_transient.hpp"
#include "Single.hpp"
#include "emit/Inline.hpp"

namespace NUClear {
namespace dsl {
    namespace word {

#ifdef _WIN32
        using event_t = long;  // NOLINT(google-runtime-int)
#else
        using event_t = short;  // NOLINT(google-runtime-int)
#endif

        /**
         * This message is sent to the IO controller to configure a new IO operation.
         */
        struct IOConfiguration {
            IOConfiguration(fd_t fd, event_t events, std::shared_ptr<threading::Reaction> reaction)
                : fd(fd), events(events), reaction(std::move(reaction)) {}
            /// The file descriptor to watch
            fd_t fd;
            /// The events to watch for on this file descriptor
            event_t events;
            /// The reaction to trigger when this file descriptor has an event
            std::shared_ptr<threading::Reaction> reaction;
        };

        /**
         * This is emitted when an IO operation has finished.
         */
        struct IOFinished {
            explicit IOFinished(const NUClear::id_t& id) : id(id) {}
            /// The id of the reaction that has finished
            NUClear::id_t id;
        };

        /**
         * This is used to trigger reactions based on standard I/O operations using file descriptors.
         *
         * @code on<IO>(file_descriptor) @endcode
         * This function works for any I/O communication which uses a file descriptor.
         * The associated reaction is triggered when the communication line matches the descriptor.
         *
         * When using this feature, runtime arguments should be provided, to specify the file descriptor.
         *
         * <b>Example Use</b>
         *
         * <b>File reading:</b> triggers a reaction when the file descriptor has data available to read.
         * @code on<IO>(pipe/stream/comms, IO::READ) @endcode
         * <b>File writing:</b> triggers a reaction when the file descriptor has data to be written.
         * @code on<IO>(pipe/stream/comms, IO::WRITE) @endcode
         * <b>File close:</b> triggers a reaction when the file descriptor is closed.
         * @code on<IO>(pipe/stream/comms, IO::CLOSE) @endcode
         * <b>File error:</b> triggers a reaction when the file descriptor reports an error.
         * @code on<IO>(pipe/stream/comms, IO::CLOSE) @endcode
         * <b>Multiple states:</b> this feature can trigger a reaction when the file descriptor matches multiple states.
         * @code on<IO>(pipe/stream/comms, IO::READ | IO::ERROR) @endcode
         *
         * @attention
         *  While a reaction is processing an IO event, no other IO triggers will fire until the reaction has completed.
         *
         * @par Implements
         *  Bind
         */
        struct IO {

// On windows we use different wait events
#ifdef _WIN32
            // NOLINTNEXTLINE(performance-enum-size) these have to be fixed types based on the api
            enum EventType : event_t {
                READ  = FD_READ | FD_OOB | FD_ACCEPT,
                WRITE = FD_WRITE,
                CLOSE = FD_CLOSE,
                ERROR = 0,
            };
#else
            // NOLINTNEXTLINE(performance-enum-size) these have to be fixed types based on the api
            enum EventType : event_t {
                READ  = POLLIN,
                WRITE = POLLOUT,
                CLOSE = POLLHUP,
                ERROR = POLLNVAL | POLLERR,
            };
#endif

            struct Event {
                /// The file descriptor that this event is for
                fd_t fd;
                /// The events that have occurred on this file descriptor
                event_t events;

                /// Returns true if the event is for the given event type
                operator bool() const {
                    return fd != INVALID_SOCKET;
                }
            };

            using ThreadEventStore = dsl::store::ThreadStore<Event>;

            template <typename DSL>
            static void bind(const std::shared_ptr<threading::Reaction>& reaction, fd_t fd, event_t watch_set) {

                reaction->unbinders.push_back([](const threading::Reaction& r) {
                    r.reactor.emit<emit::Inline>(std::make_unique<operation::Unbind<IO>>(r.id));
                });

                auto io_config = std::make_unique<IOConfiguration>(fd, watch_set, reaction);

                // Send our configuration out
                reaction->reactor.emit<emit::Inline>(io_config);
            }

            template <typename DSL>
            static Event get(const threading::ReactionTask& /*task*/) {

                // If our thread store has a value
                if (ThreadEventStore::value) {
                    return *ThreadEventStore::value;
                }

                // Otherwise return an invalid event
                return Event{INVALID_SOCKET, 0};
            }

            template <typename DSL>
            static void post_run(threading::ReactionTask& task) {
                task.parent->reactor.emit<emit::Inline>(std::make_unique<IOFinished>(task.parent->id));
            }
        };

    }  // namespace word

    namespace trait {
        template <>
        struct is_transient<word::IO::Event> : std::true_type {};
    }  // namespace trait

}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_WORD_IO_HPP
