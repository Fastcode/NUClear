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
        /// @brief The type that poll uses for events
        using event_t = long;  // NOLINT(google-runtime-int)

        /**
         * @brief A task that is waiting for an IO event
         */
        struct Task {
            Task() = default;
            Task(const fd_t& fd, event_t listening_events, std::shared_ptr<threading::Reaction> reaction)
                : fd(fd), listening_events(listening_events), reaction(std::move(reaction)) {}

            /// @brief The socket we are waiting on
            fd_t fd;
            /// @brief The events that the task is interested in
            event_t listening_events{0};
            /// @brief The events that are waiting to be fired
            event_t waiting_events{0};
            /// @brief The events that are currently being processed
            event_t processing_events{0};
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

                // Clear the waiting events, we are now processing them
                task.processing_events = task.waiting_events;
                task.waiting_events    = 0;

                // Submit the task (which should run the get)
                IO::ThreadEventStore::value                = &e;
                std::unique_ptr<threading::ReactionTask> r = task.reaction->get_task();
                IO::ThreadEventStore::value                = nullptr;

                if (r != nullptr) {
                    powerplant.submit(std::move(r));
                }
                else {
                    // Waiting events are still waiting
                    task.waiting_events |= task.processing_events;
                    task.processing_events = 0;
                }
            }
        }

        /**
         * @brief Collects the events that have happened and sets them up to fire
         */
        void process_event(const WSAEVENT& event) {

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

                    r->second.waiting_events |= wsae.lNetworkEvents;
                    fire_event(r->second);
                }
                // If we can't find the event then our list is dirty
                else {
                    dirty = true;
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

        /**
         * @brief Removes a task from the list and closes the event
         *
         * @param it the iterator to the task to remove
         *
         * @return the iterator to the next task
         */
        std::map<WSAEVENT, Task>::iterator remove_task(std::map<WSAEVENT, Task>::iterator it) {
            // Close the event
            WSAEVENT event = it->first;

            // Remove the task
            auto new_it = tasks.erase(it);

            // Try to close the WSA event
            if (!WSACloseEvent(event)) {
                throw std::system_error(WSAGetLastError(), std::system_category(), "WSACloseEvent() failed");
            }

            return new_it;
        }


    public:
        explicit IOController(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {

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
                auto it = std::find_if(tasks.begin(), tasks.end(), [&event](const std::pair<WSAEVENT, Task>& t) {
                    return t.second.reaction->id == event.id;
                });

                // If we found it then clear the waiting events
                if (it != tasks.end()) {
                    auto& task = it->second;
                    // If the events we were processing included close remove it from the list
                    if (task.processing_events & IO::CLOSE) {
                        dirty = true;
                        remove_task(it);
                    }
                    else {
                        // We have finished processing events
                        task.processing_events = 0;

                        // Try to fire again which will check if there are any waiting events
                        fire_event(task);
                    }
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
                        process_event(event);
                    }
                }
            });
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
