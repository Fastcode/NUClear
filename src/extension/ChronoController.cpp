/*
 * MIT License
 *
 * Copyright (c) 2014 NUClear Contributors
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

#include "ChronoController.hpp"

namespace NUClear {
namespace extension {

    /// The precision threshold to swap from sleeping on the condition variable to sleeping with nanosleep
    constexpr std::chrono::milliseconds precise_threshold = std::chrono::milliseconds(50);

    /**
     * Duration cast the given type to nanoseconds
     *
     * @tparam T the type to cast
     *
     * @param t the value to cast
     *
     * @return the time value in nanoseconds
     */
    template <typename T>
    std::chrono::nanoseconds ns(T&& t) {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(std::forward<T>(t));
    }

    ChronoController::ChronoController(std::unique_ptr<NUClear::Environment> environment)
        : Reactor(std::move(environment)) {

        on<Trigger<ChronoTask>>().then("Add Chrono task", [this](const std::shared_ptr<const ChronoTask>& task) {
            // Lock the mutex while we're doing stuff
            const std::lock_guard<std::mutex> lock(mutex);

            // Add our new task to the heap if we are still running
            if (running) {
                tasks.push_back(*task);
                std::push_heap(tasks.begin(), tasks.end(), std::greater<>());
            }

            // Poke the system
            wait.notify_all();
        });

        on<Trigger<dsl::operation::Unbind<ChronoTask>>>().then(
            "Unbind Chrono Task",
            [this](const dsl::operation::Unbind<ChronoTask>& unbind) {
                // Lock the mutex while we're doing stuff
                const std::lock_guard<std::mutex> lock(mutex);

                // Find the task
                auto it = std::find_if(tasks.begin(), tasks.end(), [&](const ChronoTask& task) {
                    return task.id == unbind.id;
                });

                // Remove if it exists
                if (it != tasks.end()) {
                    tasks.erase(it);
                    std::make_heap(tasks.begin(), tasks.end(), std::greater<>());
                }

                // Poke the system to make sure it's not waiting on something that's gone
                wait.notify_all();
            });

        // When we shutdown we notify so we quit now
        on<Shutdown>().then("Shutdown Chrono Controller", [this] {
            const std::lock_guard<std::mutex> lock(mutex);
            running = false;
            wait.notify_all();
        });

        on<Trigger<message::TimeTravel>>().then("Time Travel", [this](const message::TimeTravel& travel) {
            const std::lock_guard<std::mutex> lock(mutex);

            // Adjust clock to target time and leave chrono tasks where they are
            switch (travel.type) {
                case message::TimeTravel::Action::ABSOLUTE: clock::set_clock(travel.target, travel.rtf); break;
                case message::TimeTravel::Action::RELATIVE: {
                    auto adjustment = travel.target - NUClear::clock::now();
                    clock::set_clock(travel.target, travel.rtf);
                    for (auto& task : tasks) {
                        task.time += adjustment;
                    }

                } break;
                case message::TimeTravel::Action::NEAREST: {
                    const clock::time_point nearest =
                        tasks.empty() ? travel.target
                                      : std::min(travel.target, std::min_element(tasks.begin(), tasks.end())->time);
                    clock::set_clock(nearest, travel.rtf);
                } break;
            }

            // Poke the system
            wait.notify_all();
        });

        on<Always, Priority::REALTIME>().then("Chrono Controller", [this] {
            // Run until we are told to stop
            while (running) {

                // Acquire the mutex lock so we can wait on it
                std::unique_lock<std::mutex> lock(mutex);

                // If we have no chrono tasks wait until we are notified
                if (tasks.empty()) {
                    wait.wait(lock, [this] { return !running || !tasks.empty(); });
                }
                else {
                    auto start  = NUClear::clock::now();
                    auto target = tasks.front().time;

                    // Run the task if we are at or past the target time
                    if (target <= start) {
                        // Run our task and if it returns false remove it
                        const bool renew = tasks.front()();

                        // Move this to the back of the list
                        std::pop_heap(tasks.begin(), tasks.end(), std::greater<>());

                        if (renew) {
                            // Put the item back in the list
                            std::push_heap(tasks.begin(), tasks.end(), std::greater<>());
                        }
                        else {
                            // Remove the item from the list
                            tasks.pop_back();
                        }
                    }
                    // Wait if we are not at the target time
                    else {
                        // Calculate the real time to sleep given the rate at which time passes
                        const auto time_until_task = ns((target - start) / clock::rtf());

                        if (clock::rtf() == 0.0) {
                            // If we are paused then just wait until we are unpaused
                            wait.wait(lock, [&] {
                                return !running || clock::rtf() != 0.0 || NUClear::clock::now() != start;
                            });
                        }
                        else if (time_until_task > precise_threshold) {  // A long time in the future
                            // Wait on the cv
                            wait.wait_for(lock, time_until_task - precise_threshold);
                        }
                        else {  // Within precise sleep threshold
                            sleeper.sleep_for(time_until_task);
                        }
                    }
                }
            }
        });
    }

}  // namespace extension
}  // namespace NUClear
