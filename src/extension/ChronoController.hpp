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

#ifndef NUCLEAR_EXTENSION_CHRONOCONTROLLER
#define NUCLEAR_EXTENSION_CHRONOCONTROLLER

#include "../PowerPlant.hpp"
#include "../Reactor.hpp"

namespace NUClear {
namespace extension {

    class ChronoController : public Reactor {
    private:
        using ChronoTask = NUClear::dsl::operation::ChronoTask;

    public:
        explicit ChronoController(std::unique_ptr<NUClear::Environment> environment)
            : Reactor(std::move(environment)), wait_offset(std::chrono::milliseconds(0)) {

            on<Trigger<ChronoTask>>().then("Add Chrono task", [this](std::shared_ptr<const ChronoTask> task) {
                // Lock the mutex while we're doing stuff
                {
                    std::lock_guard<std::mutex> lock(mutex);

                    // Add our new task to the heap
                    tasks.push_back(*task);
                }

                // Poke the system
                wait.notify_all();
            });

            on<Trigger<dsl::operation::Unbind<ChronoTask>>>().then(
                "Unbind Chrono Task", [this](const dsl::operation::Unbind<ChronoTask>& unbind) {
                    // Lock the mutex while we're doing stuff
                    {
                        std::lock_guard<std::mutex> lock(mutex);

                        // Find the task
                        auto it = std::find_if(
                            tasks.begin(), tasks.end(), [&](const ChronoTask& task) { return task.id == unbind.id; });

                        // Remove if if it exists
                        if (it != tasks.end()) {
                            tasks.erase(it);
                        }
                    }

                    // Poke the system to make sure it's not waiting on something that's gone
                    wait.notify_all();
                });

            // When we shutdown we notify so we quit now
            on<Shutdown>().then("Shutdown Chrono Controller", [this] { wait.notify_all(); });

            on<Always, Priority::REALTIME>().then("Chrono Controller", [this] {
                // Acquire the mutex lock so we can wait on it
                std::unique_lock<std::mutex> lock(mutex);

                // If we have tasks to do
                if (!tasks.empty()) {

                    // Make the list into a heap so we can remove the soonest ones
                    std::make_heap(tasks.begin(), tasks.end(), std::greater<>());

                    // If we are within the wait offset of the time, spinlock until we get there for greater
                    // accuracy
                    if (NUClear::clock::now() + wait_offset > tasks.front().time) {

                        // Spinlock!
                        while (NUClear::clock::now() < tasks.front().time) {
                        }

                        NUClear::clock::time_point now = NUClear::clock::now();

                        // Move back from the end poping the heap
                        for (auto end = tasks.end(); end != tasks.begin() && tasks.front().time < now;) {
                            // Run our task and if it returns false remove it
                            bool renew = tasks.front()();

                            // Move this to the back of the list
                            std::pop_heap(tasks.begin(), end, std::greater<>());

                            if (!renew) {
                                end = tasks.erase(--end);
                            }
                            else {
                                --end;
                            }
                        }
                    }
                    // Otherwise we wait for the next event using a wait_for (with a small offset for greater
                    // accuracy) Either that or until we get interrupted with a new event
                    else {
                        wait.wait_until(lock, tasks.front().time - wait_offset);
                    }
                }
                // Otherwise we wait for something to happen
                else {
                    wait.wait(lock);
                }
            });
        }

    private:
        std::vector<dsl::operation::ChronoTask> tasks;
        std::mutex mutex;
        std::condition_variable wait;

        NUClear::clock::duration wait_offset;
    };

}  // namespace extension
}  // namespace NUClear

#endif  // NUCLEAR_EXTENSION_CHRONOCONTROLLER
