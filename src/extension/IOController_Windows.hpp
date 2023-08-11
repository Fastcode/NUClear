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

#ifndef NUCLEAR_EXTENSION_IOCONTROLLER_WINDOWS_HPP
#define NUCLEAR_EXTENSION_IOCONTROLLER_WINDOWS_HPP

#include "../PowerPlant.hpp"
#include "../Reactor.hpp"
#include "../dsl/word/IO.hpp"
#include "../util/platform.hpp"

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
            Task(const fd_t& fd, long events, std::shared_ptr<threading::Reaction> reaction)
                : fd(fd), events(events), reaction(std::move(reaction)) {}

            /// @brief The socket we are waiting on
            fd_t fd;
            /// @brief The events that the task is interested in
            long events{0};  // NOLINT(google-runtime-int)
            /// @brief The events that are waiting to be fired
            long waiting_events{0};  // NOLINT(google-runtime-int)
            /// @brief The reaction that is waiting for this event
            std::shared_ptr<threading::Reaction> reaction{nullptr};
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
            watches.push_back(notifier);

            for (const auto& r : tasks) {
                watches.push_back(r.first);
            }

            // We just cleaned the list!
            dirty = false;
        }

        /**
         * @brief Collects the events that have happened and stores them on the reactions to be fired
         */
        void collect_events(const WSAEVENT& event) {

            // Get the lock so we don't concurrently modify the list
            const std::lock_guard<std::mutex> lock(tasks_mutex);

            if (event == notifier) {
                // Reset the notifier signal
                if (!WSAResetEvent(event)) {
                    throw std::system_error(WSAGetLastError(),
                                            std::system_category(),
                                            "WSAResetEvent() for notifier failed");
                }
            }
            else {
                // Get our associated Event object, which has the reaction
                auto r = tasks.find(event);

                // If it was found...
                if (r != tasks.end()) {
                    // Enum the socket events to work out which ones fired
                    WSANETWORKEVENTS wsae;
                    if (WSAEnumNetworkEvents(r->second.fd, event, &wsae) == SOCKET_ERROR) {
                        throw std::system_error(WSAGetLastError(),
                                                std::system_category(),
                                                "WSAEnumNetworkEvents() failed");
                    }

                    r->second.waiting_events = r->second.waiting_events | wsae.lNetworkEvents;
                }
                // If we can't find the event then our list is dirty
                else {
                    dirty = true;
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
                auto& task = it->second;

                if (task.reaction->active_tasks == 0 && task.waiting_events != 0) {

                    // Make our event to pass through and store it in the local cache
                    IO::Event e{};
                    e.fd     = task.fd;
                    e.events = task.waiting_events;

                    // Submit the task (which should run the get)
                    try {
                        IO::ThreadEventStore::value = &e;
                        auto reaction_task          = task.reaction->get_task();
                        IO::ThreadEventStore::value = nullptr;
                        // Only actually submit this task if it is the only active task for this reaction
                        if (reaction_task) {
                            powerplant.submit(std::move(reaction_task));
                        }
                    }
                    catch (...) {
                    }

                    // Reset our value
                    it = ((task.waiting_events & IO::CLOSE) != 0) ? remove_task(it) : std::next(it);
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
            if (!WSASetEvent(notifier)) {
                throw std::system_error(WSAGetLastError(),
                                        std::system_category(),
                                        "WSASetEvent() for configure io reaction failed");
            }
        }

        std::map<WSAEVENT, Task>::iterator remove_task(std::map<WSAEVENT, Task>::iterator it) {
            // Close the event
            if (!WSACloseEvent(it->first)) {
                throw std::system_error(WSAGetLastError(), std::system_category(), "WSACloseEvent() failed");
            }

            // Remove the task
            return tasks.erase(it);
        }


    public:
        explicit IOController(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {

            // Startup WSA for IO
            WORD version = MAKEWORD(2, 2);
            WSADATA wsa_data;

            int startup_status = WSAStartup(version, &wsa_data);
            if (startup_status != 0) {
                throw std::system_error(startup_status, std::system_category(), "WSAStartup() failed");
            }

            // Create an event to use for the notifier (used for getting out of WSAWaitForMultipleEvents())
            notifier = WSACreateEvent();
            if (notifier == WSA_INVALID_EVENT) {
                throw std::system_error(WSAGetLastError(),
                                        std::system_category(),
                                        "WSACreateEvent() for notifier failed");
            }

            // Start by rebuliding the list
            rebuild_list();

            on<Trigger<dsl::word::IOConfiguration>>().then(
                "Configure IO Reaction",
                [this](const dsl::word::IOConfiguration& config) {
                    // Lock our mutex
                    std::lock_guard<std::mutex> lock(tasks_mutex);

                    // Make an event for this SOCKET
                    auto event = WSACreateEvent();
                    if (event == WSA_INVALID_EVENT) {
                        throw std::system_error(WSAGetLastError(),
                                                std::system_category(),
                                                "WSACreateEvent() for configure io reaction failed");
                    }

                    // Link the event to signal when there are events on the socket
                    if (WSAEventSelect(config.fd, event, config.events) == SOCKET_ERROR) {
                        throw std::system_error(WSAGetLastError(), std::system_category(), "WSAEventSelect() failed");
                    }

                    // Add all the information to the list and mark the list as dirty, to sync with the list of events
                    tasks.insert(std::make_pair(event, Task{config.fd, config.events, config.reaction}));
                    dirty = true;

                    bump();
                });

            on<Trigger<dsl::word::IOFinished>>().then("IO Finished", [this](const dsl::word::IOFinished& event) {
                // Get the lock so we don't concurrently modify the list
                const std::lock_guard<std::mutex> lock(tasks_mutex);

                // Find the reaction that finished processing
                auto task = std::find_if(tasks.begin(), tasks.end(), [&event](const std::pair<WSAEVENT, Task>& t) {
                    return t.second.reaction->id == event.id;
                });

                // If we found it then clear the waiting events
                if (task != tasks.end()) {
                    task->second.waiting_events = 0;
                }
            });

            on<Trigger<dsl::operation::Unbind<IO>>>().then(
                "Unbind IO Reaction",
                [this](const dsl::operation::Unbind<IO>& unbind) {
                    // Lock our mutex to avoid concurrent modification
                    const std::lock_guard<std::mutex> lock(tasks_mutex);

                    // Find our reaction
                    auto it = std::find_if(tasks.begin(), tasks.end(), [&unbind](const std::pair<WSAEVENT, Task>& t) {
                        return t.second.reaction->id == unbind.id;
                    });

                    if (it != tasks.end()) {
                        remove_task(it);
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

                    // Wait for events
                    auto event_index = WSAWaitForMultipleEvents(static_cast<DWORD>(watches.size()),
                                                                watches.data(),
                                                                false,
                                                                WSA_INFINITE,
                                                                false);

                    // Check if the return value is an event in our list
                    if (event_index >= WSA_WAIT_EVENT_0 && event_index < WSA_WAIT_EVENT_0 + watches.size()) {
                        // Get the signalled event
                        auto& event = watches[event_index - WSA_WAIT_EVENT_0];

                        // Collect the events that happened into the tasks list
                        collect_events(event);

                        // Fire the events that happened if we can
                        fire_events();
                    }
                }
            });
        }

        // We need a destructor to cleanup WSA stuff
        virtual ~IOController() {
            WSACleanup();
        }


    private:
        /// @brief The event that is used to wake up the WaitForMultipleEvents call
        WSAEVENT notifier;

        /// @brief Whether or not we are shutting down
        std::atomic<bool> shutdown{false};
        /// @brief The mutex that protects the tasks list
        std::mutex tasks_mutex;
        /// @brief Whether or not the list of file descriptors is dirty compared to tasks
        bool dirty = true;

        /// @brief The list of tasks that are currently being processed
        std::vector<WSAEVENT> watches;
        /// @brief The list of tasks that are waiting for IO events
        std::map<WSAEVENT, Task> tasks;
    };

}  // namespace extension
}  // namespace NUClear

#endif  // NUCLEAR_EXTENSION_IOCONTROLLER_WINDOWS_HPP
