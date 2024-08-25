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

#include "IOController.hpp"

namespace NUClear {
namespace extension {

    /**
     * Removes a task from the list and closes the event
     *
     * @param it the iterator to the task to remove
     *
     * @return the iterator to the next task
     */
    std::map<WSAEVENT, IOController::Task>::iterator remove_task(std::map<WSAEVENT, IOController::Task>& tasks,
                                                                 std::map<WSAEVENT, IOController::Task>::iterator it) {
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

    void IOController::rebuild_list() {
        // Get the lock so we don't concurrently modify the list
        const std::lock_guard<std::mutex> lock(tasks_mutex);

        // Clear our fds to be rebuilt
        watches.resize(0);

        // Insert our notify fd
        watches.push_back(notifier.notifier);

        for (const auto& r : tasks) {
            watches.push_back(r.first);
        }

        // We just cleaned the list!
        dirty.store(false, std::memory_order_release);
    }

    void IOController::fire_event(Task& task) {
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

    void IOController::process_event(WSAEVENT& event) {

        // Get the lock so we don't concurrently modify the list
        const std::lock_guard<std::mutex> lock(tasks_mutex);

        if (event == notifier.notifier) {
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
                    throw std::system_error(WSAGetLastError(), std::system_category(), "WSAEnumNetworkEvents() failed");
                }

                r->second.waiting_events |= wsae.lNetworkEvents;
                fire_event(r->second);
            }
            // If we can't find the event then our list is dirty
            else {
                dirty.store(true, std::memory_order_release);
            }
        }
    }

    void IOController::bump() {
        if (!WSASetEvent(notifier.notifier)) {
            throw std::system_error(WSAGetLastError(),
                                    std::system_category(),
                                    "WSASetEvent() for configure io reaction failed");
        }

        // Locking here will ensure we won't return until poll is not running
        const std::lock_guard<std::mutex> lock(notifier.mutex);
    }

    IOController::IOController(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {

        // Create an event to use for the notifier (used for getting out of WSAWaitForMultipleEvents())
        notifier.notifier = WSACreateEvent();
        if (notifier.notifier == WSA_INVALID_EVENT) {
            throw std::system_error(WSAGetLastError(), std::system_category(), "WSACreateEvent() for notifier failed");
        }

        // Start by rebuilding the list
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
                dirty.store(true, std::memory_order_release);

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
                    dirty.store(true, std::memory_order_release);
                    remove_task(tasks, it);
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
                    remove_task(tasks, it);
                }

                // Let the poll command know that stuff happened
                dirty.store(true, std::memory_order_release);
                bump();
            });

        on<Shutdown>().then("Shutdown IO Controller", [this] {
            running.store(false, std::memory_order_release);
            bump();
        });

        on<Always>().then("IO Controller", [this] {
            while (running.load(std::memory_order_acquire)) {
                // Rebuild the list if something changed
                if (dirty.load(std::memory_order_acquire)) {
                    rebuild_list();
                }

                // Wait for events
                DWORD event_index = 0;
                /*mutex scope*/ {
                    const std::lock_guard<std::mutex> lock(notifier.mutex);
                    event_index = WSAWaitForMultipleEvents(static_cast<DWORD>(watches.size()),
                                                           watches.data(),
                                                           false,
                                                           WSA_INFINITE,
                                                           false);
                }

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

}  // namespace extension
}  // namespace NUClear
