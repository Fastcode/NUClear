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
#include "../util/windows_includes.hpp"

namespace NUClear {
namespace extension {

    class IOController : public Reactor {
    public:
        explicit IOController(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {

            // Startup WSA for IO
            WORD version = MAKEWORD(2, 2);
            WSADATA wsa_data;
            WSAStartup(version, &wsa_data);

            // Reserve 1024 event slots
            // Hopefully we won't have more events than that
            // Even if we do it should be fine (after a glitch)
            fds.reserve(1024);

            // Create an event to use for the notifier
            notifier = WSACreateEvent();

            // We always have the notifier in the event list
            fds.push_back(notifier);

            on<Trigger<dsl::word::IOConfiguration>>().then(
                "Configure IO Reaction", [this](const dsl::word::IOConfiguration& config) {
                    // Lock our mutex
                    std::lock_guard<std::mutex> lock(reaction_mutex);

                    // Make an event for this SOCKET
                    auto event = WSACreateEvent();

                    // Link our event to the socket
                    WSAEventSelect(config.fd, event, config.events);

                    // Add all the information to the list
                    reactions.insert(std::make_pair(event, Event{config.fd, config.reaction, config.events}));

                    // Also add it to the end of our watching list
                    fds.push_back(event);

                    // Enable our notification event
                    WSASetEvent(notifier);
                });

            on<Trigger<dsl::operation::Unbind<IO>>>().then("Unbind IO Reaction",
                                                           [this](const dsl::operation::Unbind<IO>& unbind) {
                                                               // Lock our mutex

                                                               // Find this reaction in our list of reactions

                                                               // Remove it
                                                               // WSACloseEvent

                                                               // Flag that our list is dirty

                                                               // Enable our notification event
                                                           });

            on<Shutdown>().then("Shutdown IO Controller", [this] {
                // Set shutdown to true
                shutdown = true;

                // Enable our notification event
                WSASetEvent(notifier);
            });

            on<Always>().then("IO Controller", [this] {
                if (!shutdown) {

                    // Wait for events
                    auto event = WSAWaitForMultipleEvents(fds.size(), fds.data(), false, WSA_INFINITE, false);

                    // Check if the return value is an event in our list
                    if (event >= WSA_WAIT_EVENT_0 && event < WSA_WAIT_EVENT_0 + fds.size()) {

                        // Check for notification event
                        if (event == WSA_WAIT_EVENT_0) {
                            WSAResetEvent(notifier);
                        }
                        else {
                            // Get our event
                            auto& e = fds[event - WSA_WAIT_EVENT_0];

                            // Get our associated Event object (if it exists)
                            auto r = reactions.find(e);
                            if (r != reactions.end()) {

                                // Enum the events to work out which ones fired
                                WSANETWORKEVENTS wsae;
                                WSAEnumNetworkEvents(r->second.fd, e, &wsae);

                                // Make our event to pass through
                                IO::Event e;
                                e.fd = r->second.fd;

                                // Our events are what we got from the enum events call
                                e.events = wsae.lNetworkEvents;

                                // Store the event in our thread local cache
                                IO::ThreadEventStore::value = &e;

                                // Submit the task (which should run the get)
                                try {
                                    auto task = r->second.reaction->get_task();
                                    if (task) {
                                        powerplant.submit(std::move(task));
                                    }
                                }
                                catch (...) {
                                }

                                // Reset our value
                                IO::ThreadEventStore::value = nullptr;
                            }
                        }
                    }
                }

                // If not shudown

                // If dirty then update our list

                // If

                // WSAWaitForMultipleEvents(
                //     countOfEvents
                //     arrayofWSAEVENT
                //     waituntilall?orany // FALSE
                //     timeout // WSA_INFINITE
                //     alertable? // FALSE
                // )

                // Get the item that is associated with this event
                // Call the reaction with the information
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

        bool shutdown = false;
        std::mutex reaction_mutex;
        std::map<WSAEVENT, Event> reactions;
        std::vector<WSAEVENT> fds;
    };

}  // namespace extension
}  // namespace NUClear

#endif  // NUCLEAR_EXTENSION_IOCONTROLLER_WINDOWS_HPP
