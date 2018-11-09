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
        struct Task {
            Task() : fd(), events(0), reaction() {}
            Task(const fd_t& fd, short events, const std::shared_ptr<threading::Reaction>& reaction)
                : fd(fd), events(events), reaction(reaction) {}

            fd_t fd;
            short events;
            std::shared_ptr<threading::Reaction> reaction;

            bool operator<(const Task& other) const {
                return fd == other.fd ? events < other.events : fd < other.fd;
            }
        };

    public:
        explicit IOController(std::unique_ptr<NUClear::Environment> environment)
            : Reactor(std::move(environment)), notify_recv(), notify_send() {

            int vals[2];

            int i = pipe(static_cast<int*>(vals));
            if (i < 0) {
                throw std::system_error(
                    network_errno, std::system_category(), "We were unable to make the notification pipe for IO");
            }

            notify_recv = vals[0];
            notify_send = vals[1];

            // Add our notification pipe to our list of fds
            fds.push_back(pollfd{notify_recv, POLLIN, 0});

            on<Trigger<dsl::word::IOConfiguration>>().then(
                "Configure IO Reaction", [this](const dsl::word::IOConfiguration& config) {
                    // Lock our mutex to avoid concurrent modification
                    std::lock_guard<std::mutex> lock(reaction_mutex);

                    reactions.emplace_back(config.fd, static_cast<short>(config.events), config.reaction);

                    // Resort our list
                    std::sort(std::begin(reactions), std::end(reactions));

                    // Let the poll command know that stuff happened
                    dirty = true;

                    // Check if there was an error
                    if (write(notify_send, &dirty, 1) < 0) {
                        throw std::system_error(network_errno,
                                                std::system_category(),
                                                "There was an error while writing to the notification pipe");
                    }
                });

            on<Trigger<dsl::operation::Unbind<IO>>>().then(
                "Unbind IO Reaction", [this](const dsl::operation::Unbind<IO>& unbind) {
                    // Lock our mutex to avoid concurrent modification
                    std::lock_guard<std::mutex> lock(reaction_mutex);

                    // Find our reaction
                    auto reaction = std::find_if(std::begin(reactions), std::end(reactions), [&unbind](const Task& t) {
                        return t.reaction->id == unbind.id;
                    });

                    if (reaction != std::end(reactions)) { reactions.erase(reaction); }

                    // Let the poll command know that stuff happened
                    dirty = true;
                    if (write(notify_send, &dirty, 1) < 0) {
                        throw std::system_error(network_errno,
                                                std::system_category(),
                                                "There was an error while writing to the notification pipe");
                    }
                });

            on<Shutdown>().then("Shutdown IO Controller", [this] {
                // Set shutdown to true so it won't try to poll again
                shutdown = true;
                // A byte to send down the pipe
                char val = 0;

                // Send a single byte down the pipe
                if (write(notify_send, &val, 1) < 0) {
                    throw std::system_error(network_errno,
                                            std::system_category(),
                                            "There was an error while writing to the notification pipe");
                }
            });

            on<Always>().then("IO Controller", [this] {
                // To make sure we don't get caught in a weird loop
                // shutdown keeps us out here
                if (!shutdown) {


                    // TODO(trent): check for dirty here


                    // Poll our file descriptors for events
                    int result = poll(fds.data(), static_cast<nfds_t>(fds.size()), -1);

                    // Check if we had an error on our Poll request
                    if (result < 0) {
                        throw std::system_error(network_errno,
                                                std::system_category(),
                                                "There was an IO error while attempting to poll the file descriptors");
                    }
                    else {
                        for (auto& fd : fds) {

                            // Something happened
                            if (fd.revents != 0) {

                                // It's our notification handle
                                if (fd.fd == notify_recv) {
                                    // Read our value to clear it's read status
                                    char val;
                                    if (read(fd.fd, &val, sizeof(char)) < 0) {
                                        throw std::system_error(network_errno,
                                                                std::system_category(),
                                                                "There was an error reading our notification pipe?");
                                    };
                                }
                                // It's a regular handle
                                else {

                                    // Find our relevant reactions
                                    auto range =
                                        std::equal_range(std::begin(reactions),
                                                         std::end(reactions),
                                                         Task{fd.fd, 0, nullptr},
                                                         [](const Task& a, const Task& b) { return a.fd < b.fd; });


                                    // There are no reactions for this!
                                    if (range.first == std::end(reactions)) {
                                        // If this happens then our list is definitely dirty...
                                        dirty = true;
                                    }
                                    else {


                                        // TODO(trent): we also want to swap this element to the back of the list and
                                        // remove it so that it does not fire again


                                        // Loop through our values
                                        for (auto it = range.first; it != range.second; ++it) {

                                            // We should emit if the reaction is interested
                                            if ((it->events & fd.revents) != 0) {

                                                // Make our event to pass through
                                                IO::Event e;
                                                e.fd = fd.fd;

                                                // Evaluate and store our set in thread store
                                                e.events = fd.revents;

                                                // Store the event in our thread local cache
                                                IO::ThreadEventStore::value = &e;

                                                // Submit the task (which should run the get)
                                                try {
                                                    auto task = it->reaction->get_task();
                                                    if (task) { powerplant.submit(std::move(task)); }
                                                }
                                                catch (...) {
                                                }

                                                // Reset our value
                                                IO::ThreadEventStore::value = nullptr;

                                                // TODO(trent): If we had a close, or error stop listening?
                                            }
                                        }
                                    }
                                }

                                // Reset our events
                                fd.revents = 0;
                            }
                        }

                        // If our list is dirty
                        if (dirty) {
                            // Get the lock so we don't concurrently modify the list
                            std::lock_guard<std::mutex> lock(reaction_mutex);

                            // Clear our fds to be rebuilt
                            fds.resize(0);

                            // Insert our notify fd
                            fds.push_back(pollfd{notify_recv, POLLIN, 0});

                            for (const auto& r : reactions) {

                                // If we are the same fd, then add our interest set
                                if (r.fd == fds.back().fd) { fds.back().events |= r.events; }
                                // Otherwise add a new one
                                else {
                                    fds.push_back(pollfd{r.fd, r.events, 0});
                                }
                            }

                            // We just cleaned the list!
                            dirty = false;
                        }
                    }
                }
            });
        }

    private:
        fd_t notify_recv;
        fd_t notify_send;

        bool shutdown = false;
        bool dirty    = true;
        std::mutex reaction_mutex;
        std::vector<pollfd> fds;
        std::vector<Task> reactions;
    };

}  // namespace extension
}  // namespace NUClear

#endif  // NUCLEAR_EXTENSION_IOCONTROLLER_POSIX_HPP
