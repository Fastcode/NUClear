/*
 * MIT License
 *
 * Copyright (c) 2016 NUClear Contributors
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

#ifndef NUCLEAR_EXTENSION_IO_CONTROLLER_WINDOWS_HPP
#define NUCLEAR_EXTENSION_IO_CONTROLLER_WINDOWS_HPP

#include "../PowerPlant.hpp"
#include "../Reactor.hpp"
#include "../dsl/word/IO.hpp"
#include "../util/platform.hpp"

namespace NUClear {
namespace extension {

    class IOController : public Reactor {
    private:
/// The type that poll uses for events
#ifdef _WIN32
        using event_t = long;  // NOLINT(google-runtime-int)
#else
        using event_t = decltype(pollfd::events);
#endif

        /**
         * A task that is waiting for an IO event.
         */
        struct Task {
            Task() = default;
            Task(const fd_t& fd, event_t listening_events, std::shared_ptr<threading::Reaction> reaction)
                : fd(fd), listening_events(listening_events), reaction(std::move(reaction)) {}

            /// The file descriptor we are waiting on
            fd_t fd{INVALID_SOCKET};
            /// The events that the task is interested in
            event_t listening_events{0};
            /// The events that are waiting to be fired
            event_t waiting_events{0};
            /// The events that are currently being processed
            event_t processing_events{0};
            /// The reaction that is waiting for this event
            std::shared_ptr<threading::Reaction> reaction{nullptr};

            /**
             * Sorts the tasks by their file descriptor
             *
             * The tasks are sorted by file descriptor so that when we rebuild the list of file descriptors to poll we
             * can assume that if the same file descriptor shows up multiple times it will be next to each other. This
             * allows the events that are being watched to be or'ed together.
             *
             * @param other  the other task to compare to
             *
             * @return true  if this task is less than the other
             * @return false if this task is greater than or equal to the other
             */
            bool operator<(const Task& other) const {
                return fd == other.fd ? listening_events < other.listening_events : fd < other.fd;
            }
        };

        /**
         * Rebuilds the list of file descriptors to poll.
         *
         * This function is called when the list of file descriptors to poll changes.
         * It will rebuild the list of file descriptors used by poll.
         */
        void rebuild_list();

        /**
         * Fires the event for the task if it is ready.
         *
         * @param task The task to try to fire the event for
         */
        void fire_event(Task& task);

        /**
         * Collects the events that have happened and sets them up to fire.
         */
        void process_event(const WSAEVENT& event);

        /**
         * Bumps the notification pipe to wake up the poll command.
         *
         * If the poll command is waiting it will wait forever if something doesn't happen.
         * When trying to update what to poll or shut down we need to wake it up so it can.
         */
        // NOLINTNEXTLINE(readability-make-member-function-const) this changes states
        void bump();

        /**
         * Removes a task from the list and closes the event.
         *
         * @param it The iterator to the task to remove
         *
         * @return The iterator to the next task
         */
        std::map<WSAEVENT, Task>::iterator remove_task(std::map<WSAEVENT, Task>::iterator it);

    public:
        explicit IOController(std::unique_ptr<NUClear::Environment> environment);

    private:
        /// The event that is used to wake up the WaitForMultipleEvents call
        WSAEVENT notifier;

        /// Whether or not we are shutting down
        std::atomic<bool> shutdown{false};
        /// The mutex that protects the tasks list
        std::mutex tasks_mutex;
        /// Whether or not the list of file descriptors is dirty compared to tasks
        bool dirty = true;

        /// The list of tasks that are currently being processed
        std::vector<WSAEVENT> watches;
        /// The list of tasks that are waiting for IO events
        std::map<WSAEVENT, Task> tasks;
    };

}  // namespace extension
}  // namespace NUClear

#endif  // NUCLEAR_EXTENSION_IO_CONTROLLER_WINDOWS_HPP
