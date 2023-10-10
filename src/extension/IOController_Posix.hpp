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

#ifndef NUCLEAR_EXTENSION_IOCONTROLLER_POSIX_HPP
#define NUCLEAR_EXTENSION_IOCONTROLLER_POSIX_HPP

#include <poll.h>
#include <unistd.h>

#include <algorithm>
#include <mutex>
#include <system_error>

#include "../PowerPlant.hpp"
#include "../Reactor.hpp"
#include "../dsl/word/IO.hpp"

namespace NUClear {
namespace extension {

    class IOController : public Reactor {
    private:
        /// @brief The type that poll uses for events
        using event_t = decltype(pollfd::events);

        /**
         * @brief A task that is waiting for an IO event
         */
        struct Task {
            Task() = default;
            // NOLINTNEXTLINE(google-runtime-int)
            Task(const fd_t& fd, event_t listening_events, std::shared_ptr<threading::Reaction> reaction)
                : fd(fd), listening_events(listening_events), reaction(std::move(reaction)) {}

            /// @brief The file descriptor we are waiting on
            fd_t fd{-1};
            /// @brief The events that the task is interested in
            event_t listening_events{0};
            /// @brief The events that are waiting to be fired
            event_t waiting_events{0};
            /// @brief The events that are currently being processed
            event_t processing_events{0};
            /// @brief The reaction that is waiting for this event
            std::shared_ptr<threading::Reaction> reaction{nullptr};

            /**
             * @brief Sorts the tasks by their file descriptor
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
         * @brief Rebuilds the list of file descriptors to poll
         *
         * This function is called when the list of file descriptors to poll changes. It will rebuild the list of file
         * descriptors used by poll
         */
        void rebuild_list() {
            // Get the lock so we don't concurrently modify the list
            const std::lock_guard<std::mutex> lock(tasks_mutex);

            // Clear our fds to be rebuilt
            watches.resize(0);

            // Insert our notify fd
            watches.push_back(pollfd{notify_recv, POLLIN, 0});

            for (const auto& r : tasks) {
                // If we are the same fd, then add our interest set and mask out events that are already being processed
                if (r.fd == watches.back().fd) {
                    watches.back().events = event_t((watches.back().events | r.listening_events)
                                                    & ~(r.processing_events | r.waiting_events));
                }
                // Otherwise add a new one and mask out events that are already being processed
                else {
                    watches.push_back(
                        pollfd{r.fd, event_t(r.listening_events & ~(r.processing_events | r.waiting_events)), 0});
                }
            }

            // We just cleaned the list!
            dirty = false;
        }

        /**
         * @brief Fires the event for the task if it is ready
         *
         * @param task the task to try to fire the event for
         */
        void fire_event(Task& task) {
            if (task.processing_events == 0 && task.waiting_events != 0) {

                // Make our event to pass through and store it in the local cache
                IO::Event e{};
                e.fd     = task.fd;
                e.events = task.waiting_events;

                // Submit the task (which should run the get)
                IO::ThreadEventStore::value                = &e;
                std::unique_ptr<threading::ReactionTask> r = task.reaction->get_task();
                IO::ThreadEventStore::value                = nullptr;

                if (r != nullptr) {
                    // Clear the waiting events, we are now processing them
                    task.processing_events = task.waiting_events;
                    task.waiting_events    = 0;

                    // Mask out the currently processing events so poll doesn't notify for them
                    auto it =
                        std::lower_bound(watches.begin(), watches.end(), task.fd, [&](const pollfd& w, const fd_t& fd) {
                            return w.fd < fd;
                        });
                    if (it != watches.end() && it->fd == task.fd) {
                        it->events = event_t(it->events & ~task.processing_events);
                    }

                    powerplant.submit(std::move(r));
                }
                else {
                    task.waiting_events    = event_t(task.waiting_events | task.processing_events);
                    task.processing_events = 0;
                }
            }
        }

        /**
         * @brief Collects the events that have happened and sets them up to fire
         */
        void process_events() {

            // Get the lock so we don't concurrently modify the list
            const std::lock_guard<std::mutex> lock(tasks_mutex);

            for (auto& fd : watches) {

                // Something happened
                if (fd.revents != 0) {

                    // It's our notification handle
                    if (fd.fd == notify_recv) {
                        // Read our value to clear it's read status
                        char val = 0;
                        if (::read(fd.fd, &val, sizeof(char)) < 0) {
                            throw std::system_error(network_errno,
                                                    std::system_category(),
                                                    "There was an error reading our notification pipe?");
                        }
                    }
                    // It's a regular handle
                    else {
                        // Check if we have a read event but 0 bytes to read, this can happen when a socket is closed
                        // On linux we don't get a close event, we just keep getting read events with 0 bytes
                        // To make the close event happen if we get a read event with 0 bytes we will check if there are
                        // any currently processing reads and if not, then close
                        bool maybe_eof = false;
                        if ((fd.revents & IO::READ) != 0) {
                            int bytes_available = 0;
                            const bool valid    = ::ioctl(fd.fd, FIONREAD, &bytes_available) == 0;
                            if (valid && bytes_available == 0) {
                                maybe_eof = true;
                            }
                        }

                        // Find our relevant tasks
                        auto range = std::equal_range(tasks.begin(),
                                                      tasks.end(),
                                                      Task{fd.fd, 0, nullptr},
                                                      [](const Task& a, const Task& b) { return a.fd < b.fd; });

                        // There are no tasks for this!
                        if (range.first == tasks.end()) {
                            // If this happens then our list is definitely dirty...
                            dirty = true;
                        }
                        else {
                            // Loop through our values
                            for (auto it = range.first; it != range.second; ++it) {
                                // Load in the relevant events that happened into the waiting events
                                it->waiting_events = event_t(it->waiting_events | (it->listening_events & fd.revents));

                                if (maybe_eof && (it->processing_events & IO::READ) == 0) {
                                    it->waiting_events |= IO::CLOSE;
                                }

                                fire_event(*it);
                            }
                        }
                    }

                    // Clear the events from poll to avoid double firing
                    fd.revents = 0;
                }
            }
        }

        /**
         * @brief Bumps the notification pipe to wake up the poll command
         *
         * If the poll command is waiting it will wait forever if something doesn't happen.
         * When trying to update what to poll or shut down we need to wake it up so it can.
         */
        // NOLINTNEXTLINE(readability-make-member-function-const) this changes states
        void bump() {
            // Check if there was an error
            uint8_t val = 1;
            if (::write(notify_send, &val, sizeof(val)) < 0) {
                throw std::system_error(network_errno,
                                        std::system_category(),
                                        "There was an error while writing to the notification pipe");
            }

            // Locking here will ensure we won't return until poll is not running
            const std::lock_guard<std::mutex> lock(poll_mutex);
        }

    public:
        explicit IOController(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {

            std::array<int, 2> vals = {-1, -1};
            const int i             = ::pipe(vals.data());
            if (i < 0) {
                throw std::system_error(network_errno,
                                        std::system_category(),
                                        "We were unable to make the notification pipe for IO");
            }
            notify_recv = vals[0];
            notify_send = vals[1];

            // Start by rebuilding the list
            rebuild_list();

            on<Trigger<dsl::word::IOConfiguration>>().then(
                "Configure IO Reaction",
                [this](const dsl::word::IOConfiguration& config) {
                    // Lock our mutex to avoid concurrent modification
                    const std::lock_guard<std::mutex> lock(tasks_mutex);

                    // NOLINTNEXTLINE(google-runtime-int)
                    tasks.emplace_back(config.fd, event_t(config.events), config.reaction);

                    // Resort our list
                    std::sort(tasks.begin(), tasks.end());

                    // Let the poll command know that stuff happened
                    dirty = true;
                    bump();
                });

            on<Trigger<dsl::word::IOFinished>>().then("IO Finished", [this](const dsl::word::IOFinished& event) {
                // Get the lock so we don't concurrently modify the list
                const std::lock_guard<std::mutex> lock(tasks_mutex);

                // Find the reaction that finished processing
                auto task = std::find_if(tasks.begin(), tasks.end(), [&event](const Task& t) {
                    return t.reaction->id == event.id;
                });

                if (task != tasks.end()) {
                    // If the events we were processing included close remove it from the list
                    if ((task->processing_events & IO::CLOSE) != 0) {
                        dirty = true;
                        tasks.erase(task);
                    }
                    else {
                        // Make sure poll isn't currently waiting for an event to happen
                        bump();

                        // Unmask the events that were just processed
                        auto it = std::lower_bound(watches.begin(),
                                                   watches.end(),
                                                   task->fd,
                                                   [&](const pollfd& w, const fd_t& fd) { return w.fd < fd; });
                        if (it != watches.end() && it->fd == task->fd) {
                            it->events = event_t(it->events | task->processing_events);
                        }

                        // No longer processing events
                        task->processing_events = 0;

                        // Try to fire again which will check if there are any waiting events
                        fire_event(*task);
                    }
                }
            });

            on<Trigger<dsl::operation::Unbind<IO>>>().then(
                "Unbind IO Reaction",
                [this](const dsl::operation::Unbind<IO>& unbind) {
                    // Lock our mutex to avoid concurrent modification
                    const std::lock_guard<std::mutex> lock(tasks_mutex);

                    // Find our reaction
                    auto reaction = std::find_if(tasks.begin(), tasks.end(), [&unbind](const Task& t) {
                        return t.reaction->id == unbind.id;
                    });

                    if (reaction != tasks.end()) {
                        tasks.erase(reaction);
                    }

                    // Let the poll command know that stuff happened
                    dirty = true;
                    bump();
                });

            on<Shutdown>().then("Shutdown IO Controller", [this] {
                // Set shutdown to true so it won't try to poll again
                shutdown.store(true);
                bump();
            });

            on<Always>().then("IO Controller", [this] {
                // To make sure we don't get caught in a weird loop
                // shutdown keeps us out here
                if (!shutdown.load()) {

                    // Rebuild the list if something changed
                    if (dirty) {
                        rebuild_list();
                    }

                    // Wait for an event to happen on one of our file descriptors
                    /* mutex scope */ {
                        const std::lock_guard<std::mutex> lock(poll_mutex);
                        if (::poll(watches.data(), nfds_t(watches.size()), -1) < 0) {
                            throw std::system_error(
                                network_errno,
                                std::system_category(),
                                "There was an IO error while attempting to poll the file descriptors");
                        }
                    }

                    // Collect the events that happened into the tasks list
                    process_events();
                }
            });
        }

    private:
        /// @brief The receive file descriptor for our notification pipe
        fd_t notify_recv{-1};
        /// @brief The send file descriptor for our notification pipe
        fd_t notify_send{-1};

        /// @brief The mutex to wait on when bumping to ensure poll has returned
        std::mutex poll_mutex;

        /// @brief Whether or not we are shutting down
        std::atomic<bool> shutdown{false};
        /// @brief The mutex that protects the tasks list
        std::mutex tasks_mutex;
        /// @brief Whether or not the list of file descriptors is dirty compared to tasks
        bool dirty = true;
        /// @brief The list of file descriptors to poll
        std::vector<pollfd> watches{};
        /// @brief The list of tasks that are waiting for IO events
        std::vector<Task> tasks{};
    };

}  // namespace extension
}  // namespace NUClear

#endif  // NUCLEAR_EXTENSION_IOCONTROLLER_POSIX_HPP
