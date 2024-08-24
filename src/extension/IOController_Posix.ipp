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

#include "IOController.hpp"

namespace NUClear {
namespace extension {

    void IOController::rebuild_list() {
        // Get the lock so we don't concurrently modify the list
        const std::lock_guard<std::mutex> lock(tasks_mutex);

        // Clear our fds to be rebuilt
        watches.resize(0);

        // Insert our notify fd
        watches.push_back(pollfd{notifier.recv, POLLIN | POLLERR | POLLNVAL, 0});

        for (const auto& r : tasks) {
            // If we are the same fd, then add our interest set and mask out events that are already being processed
            if (r.fd == watches.back().fd) {
                watches.back().events =
                    event_t((watches.back().events | r.listening_events) & ~(r.processing_events | r.waiting_events));
            }
            // Otherwise add a new one and mask out events that are already being processed
            else {
                watches.push_back(
                    pollfd{r.fd, event_t(r.listening_events & ~(r.processing_events | r.waiting_events)), 0});
            }
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

    void IOController::process_event(pollfd& event) {


        // It's our notification handle
        if (event.fd == notifier.recv) {
            // Read our value to clear it's read status
            char val = 0;
            if (::read(event.fd, &val, sizeof(char)) < 0) {
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
            if ((event.revents & IO::READ) != 0) {
                int bytes_available = 0;
                const bool valid    = ::ioctl(event.fd, FIONREAD, &bytes_available) == 0;
                if (valid && bytes_available == 0) {
                    maybe_eof = true;
                }
            }

            // Find our relevant tasks
            auto range = std::equal_range(tasks.begin(),
                                          tasks.end(),
                                          Task{event.fd, 0, nullptr},
                                          [](const Task& a, const Task& b) { return a.fd < b.fd; });

            // There are no tasks for this!
            if (range.first == tasks.end()) {
                // If this happens then our list is definitely dirty...
                dirty.store(true, std::memory_order_release);
            }
            else {
                // Loop through our values
                for (auto it = range.first; it != range.second; ++it) {
                    // Load in the relevant events that happened into the waiting events
                    it->waiting_events = event_t(it->waiting_events | (it->listening_events & event.revents));

                    if (maybe_eof && (it->processing_events & IO::READ) == 0) {
                        it->waiting_events |= IO::CLOSE;
                    }

                    fire_event(*it);
                }
            }
        }

        // Clear the events from poll to avoid double firing
        event.revents = 0;
    }

    void IOController::bump() {
        // Check if there was an error
        uint8_t val = 1;
        if (::write(notifier.send, &val, sizeof(val)) < 0) {
            throw std::system_error(network_errno,
                                    std::system_category(),
                                    "There was an error while writing to the notification pipe");
        }

        // Locking here will ensure we won't return until poll is not running
        const std::lock_guard<std::mutex> lock(notifier.mutex);
    }

    IOController::IOController(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {

        std::array<int, 2> vals = {-1, -1};
        const int i             = ::pipe(vals.data());
        if (i < 0) {
            throw std::system_error(network_errno,
                                    std::system_category(),
                                    "We were unable to make the notification pipe for IO");
        }
        notifier.recv = vals[0];
        notifier.send = vals[1];

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
                dirty.store(true, std::memory_order_release);
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
                    dirty.store(true, std::memory_order_release);
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
                dirty.store(true, std::memory_order_release);
                bump();
            });

        on<Shutdown>().then("Shutdown IO Controller", [this] {
            running.store(false, std::memory_order_release);
            bump();
        });

        on<Always>().then("IO Controller", [this] {
            // Stay in this reaction to improve the performance without going back/fourth between reactions
            if (running.load(std::memory_order_acquire)) {
                // Rebuild the list if something changed
                if (dirty.load(std::memory_order_acquire)) {
                    rebuild_list();
                }

                // Wait for an event to happen on one of our file descriptors
                /* mutex scope */ {
                    const std::lock_guard<std::mutex> lock(notifier.mutex);
                    if (::poll(watches.data(), nfds_t(watches.size()), -1) < 0) {
                        throw std::system_error(network_errno,
                                                std::system_category(),
                                                "There was an IO error while attempting to poll the file descriptors");
                    }
                }

                // Get the lock so we don't concurrently modify the list
                const std::lock_guard<std::mutex> lock(tasks_mutex);
                for (auto& fd : watches) {

                    // Collect the events that happened into the tasks list
                    // Something happened
                    if (fd.revents != 0) {
                        process_event(fd);
                    }
                }
            }
        });
    }

}  // namespace extension
}  // namespace NUClear
