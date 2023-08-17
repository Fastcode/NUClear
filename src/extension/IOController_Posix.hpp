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
        /**
         * @brief A task that is waiting for an IO event
         */
        struct Task {
            Task() = default;
            // NOLINTNEXTLINE(google-runtime-int)
            Task(const fd_t& fd, short events, std::shared_ptr<threading::Reaction> reaction)
                : fd(fd), events(events), reaction(std::move(reaction)) {}

            /// @brief The file descriptor we are waiting on
            fd_t fd{-1};
            /// @brief The events that are waiting to be fired
            short waiting_events{0};  // NOLINT(google-runtime-int)
            /// @brief The events that the task is interested in
            short events{0};  // NOLINT(google-runtime-int)
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
                return fd == other.fd ? events < other.events : fd < other.fd;
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
                // If we are the same fd, then add our interest set
                if (r.fd == watches.back().fd) {
                    watches.back().events = short(watches.back().events | r.events);  // NOLINT(google-runtime-int)
                }
                // Otherwise add a new one
                else {
                    watches.push_back(pollfd{r.fd, r.events, 0});
                }
            }

            // We just cleaned the list!
            dirty = false;
        }

        /**
         * @brief Collects the events that have happened and stores them on the reactions to be fired
         */
        void collect_events() {

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
                        };
                    }
                    // It's a regular handle
                    else {
                        // Check how many bytes are available to read, if it's 0 and we have a read event the
                        // descriptor is sending EOF and we should fire a CLOSE event too and stop watching
                        if ((fd.revents & IO::READ) != 0) {
                            int bytes_available = 0;
                            const bool valid    = ::ioctl(fd.fd, FIONREAD, &bytes_available) == 0;
                            if (valid && bytes_available == 0) {
                                // NOLINTNEXTLINE(google-runtime-int)
                                fd.revents = short(fd.revents | IO::CLOSE);
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
                                // NOLINTNEXTLINE(google-runtime-int)
                                it->waiting_events = short(it->waiting_events | (it->events & fd.revents));
                            }
                        }
                    }

                    // Clear the events from poll to avoid double firing
                    fd.revents = 0;
                }
            }
        }

        /**
         * @brief Fires the events that have been collected when the reactions are ready
         */
        void fire_events() {
            const std::lock_guard<std::mutex> lock(tasks_mutex);

            // Go through every reaction and if it has events and isn't already running then run it
            for (auto it = tasks.begin(); it != tasks.end();) {
                if (it->reaction->active_tasks == 0 && it->waiting_events != 0) {

                    // Make our event to pass through and store it in the local cache
                    IO::Event e{};
                    e.fd     = it->fd;
                    e.events = it->waiting_events;

                    // Submit the task (which should run the get)
                    IO::ThreadEventStore::value = &e;
                    powerplant.submit(it->reaction->get_task());
                    IO::ThreadEventStore::value = nullptr;

                    // Remove if we received a close event
                    const bool closed = (it->waiting_events & IO::CLOSE) != 0;
                    dirty |= closed;
                    it = closed ? tasks.erase(it) : std::next(it);
                }
                else {
                    ++it;
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

            // Start by rebuliding the list
            rebuild_list();

            on<Trigger<dsl::word::IOConfiguration>>().then(
                "Configure IO Reaction",
                [this](const dsl::word::IOConfiguration& config) {
                    // Lock our mutex to avoid concurrent modification
                    const std::lock_guard<std::mutex> lock(tasks_mutex);

                    // NOLINTNEXTLINE(google-runtime-int)
                    tasks.emplace_back(config.fd, short(config.events), config.reaction);

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

                // If we found it then clear the waiting events
                if (task != tasks.end()) {
                    task->waiting_events = 0;
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
                    if (::poll(watches.data(), nfds_t(watches.size()), -1) < 0) {
                        throw std::system_error(network_errno,
                                                std::system_category(),
                                                "There was an IO error while attempting to poll the file descriptors");
                    }

                    // Collect the events that happened into the tasks list
                    collect_events();

                    // Fire the events that happened if we can
                    fire_events();
                }
            });
        }

    private:
        /// @brief The receive file descriptor for our notification pipe
        fd_t notify_recv{-1};
        /// @brief The send file descriptor for our notification pipe
        fd_t notify_send{-1};

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
