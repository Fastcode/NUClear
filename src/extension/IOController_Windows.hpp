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
    public:
        explicit IOController(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {

            // Startup WSA for IO
            WORD version = MAKEWORD(2, 2);
            WSADATA wsa_data;

            int startup_status = WSAStartup(version, &wsa_data);
            if (startup_status != 0) {
                throw std::system_error(startup_status, std::system_category(), "WSAStartup() failed");
            }

            // Reserve 1024 event slots
            // Hopefully we won't have more events than that
            // Even if we do it should be fine (after a glitch)
            events.reserve(1024);

            // Create an event to use for the notifier (used for getting out of WSAWaitForMultipleEvents())
            notifier = WSACreateEvent();
            if (notifier == WSA_INVALID_EVENT) {
                throw std::system_error(
                    WSAGetLastError(), std::system_category(), "WSACreateEvent() for notifier failed");
            }

            // We always have the notifier in the event list
            events.push_back(notifier);

            on<Trigger<dsl::word::IOConfiguration>>().then(
                "Configure IO Reaction", [this](const dsl::word::IOConfiguration& config) {
                    // Lock our mutex
                    std::lock_guard<std::mutex> lock(reaction_mutex);

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
                    reactions.insert(std::make_pair(event, Event{config.fd, config.reaction, config.events}));
                    reactions_list_dirty = true;

                    // Signal the notifier event to return from WSAWaitForMultipleEvents() and sync the dirty list
                    if (!WSASetEvent(notifier)) {
                        throw std::system_error(WSAGetLastError(),
                                                std::system_category(),
                                                "WSASetEvent() for configure io reaction failed");
                    }
                });

            on<Trigger<dsl::operation::Unbind<IO>>>().then(
                "Unbind IO Reaction", [this](const dsl::operation::Unbind<IO>& unbind) {
                    // Lock our mutex
                    std::lock_guard<std::mutex> lock(reaction_mutex);

                    // Find this reaction in our list of reactions
                    auto reaction = std::find_if(
                        std::begin(reactions), std::end(reactions), [&unbind](const std::pair<WSAEVENT, Event>& item) {
                            return item.second.reaction->id == unbind.id;
                        });

                    // If the reaction was found
                    if (reaction != std::end(reactions)) {
                        // Remove it from the list of reactions
                        reactions.erase(reaction);

                        // Queue the associated event for closing when we sync
                        events_to_close.push_back(reaction->first);
                    }
                    else {
                        // Fail silently: we've unbound a reaction that somehow isn't in our list of reactions!
                    }

                    // Flag that our list is dirty
                    reactions_list_dirty = true;

                    // Signal the notifier event to return from WSAWaitForMultipleEvents() and sync the dirty list
                    if (!WSASetEvent(notifier)) {
                        throw std::system_error(
                            WSAGetLastError(), std::system_category(), "WSASetEvent() for unbind io reaction failed");
                    }
                });

            on<Shutdown>().then("Shutdown IO Controller", [this] {
                // Set shutdown to true
                shutdown = true;

                // Signal the notifier event to return from WSAWaitForMultipleEvents() and shutdown
                if (!WSASetEvent(notifier)) {
                    throw std::system_error(
                        WSAGetLastError(), std::system_category(), "WSASetEvent() for shutdown failed");
                }
            });

            on<Always>().then("IO Controller", [this] {
                if (!shutdown) {
                    // Wait for events
                    auto event_index = WSAWaitForMultipleEvents(
                        static_cast<DWORD>(events.size()), events.data(), false, WSA_INFINITE, false);

                    // Check if the return value is an event in our list
                    if (event_index >= WSA_WAIT_EVENT_0 && event_index < WSA_WAIT_EVENT_0 + events.size()) {
                        // Get the signalled event
                        auto& event = events[event_index - WSA_WAIT_EVENT_0];

                        if (event == notifier) {
                            // Reset the notifier signal
                            if (!WSAResetEvent(event)) {
                                throw std::system_error(
                                    WSAGetLastError(), std::system_category(), "WSAResetEvent() for notifier failed");
                            }
                        }
                        else {
                            // Get our associated Event object, which has the reaction
                            auto r = reactions.find(event);

                            // If it was found...
                            if (r != reactions.end()) {
                                // Enum the socket events to work out which ones fired
                                WSANETWORKEVENTS wsae;
                                if (WSAEnumNetworkEvents(r->second.fd, event, &wsae) == SOCKET_ERROR) {
                                    throw std::system_error(
                                        WSAGetLastError(), std::system_category(), "WSAEnumNetworkEvents() failed");
                                }

                                // Make our IO event to pass through
                                IO::Event io_event;
                                io_event.fd = r->second.fd;

                                // The events that fired are what we got from the enum events call
                                io_event.events = wsae.lNetworkEvents;

                                // Store the IO event in our thread local cache
                                IO::ThreadEventStore::value = &io_event;

                                // Submit the task (which should run the get)
                                try {
                                    auto task = r->second.reaction->get_task();
                                    if (task) { powerplant.submit(std::move(task)); }
                                }
                                catch (...) {
                                }

                                // Reset our value
                                IO::ThreadEventStore::value = nullptr;
                            }
                        }
                    }
                }

                if (reactions_list_dirty || !events_to_close.empty()) {
                    // Get the lock so we don't concurrently modify the list
                    std::lock_guard<std::mutex> lock(reaction_mutex);

                    // Close any events we've queued for closing
                    if (!events_to_close.empty()) {
                        for (auto& event : events_to_close) {
                            if (!WSACloseEvent(event)) {
                                throw std::system_error(
                                    WSAGetLastError(), std::system_category(), "WSACloseEvent() failed");
                            }
                        }

                        // Clear the queue of closed events
                        events_to_close.clear();
                    }

                    // Clear the list of events, to be rebuilt
                    events.resize(0);

                    // Add back the notifier event
                    events.push_back(notifier);

                    // Sync the list of reactions to the list of events
                    for (const auto& r : reactions) {
                        events.push_back(r.first);
                    }

                    // The list has been synced
                    reactions_list_dirty = false;
                }
            });
        }

        // We need a destructor to cleanup WSA stuff
        virtual ~IOController() {
            WSACleanup();
        }

    private:
        struct Event {
            SOCKET fd;
            std::shared_ptr<threading::Reaction> reaction;
            int events;
        };

        WSAEVENT notifier;

        bool shutdown             = false;
        bool reactions_list_dirty = false;

        std::mutex reaction_mutex;
        std::map<WSAEVENT, Event> reactions;
        std::vector<WSAEVENT> events;
        std::vector<WSAEVENT> events_to_close;
    };

}  // namespace extension
}  // namespace NUClear

#endif  // NUCLEAR_EXTENSION_IOCONTROLLER_WINDOWS_HPP
