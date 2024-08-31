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

#include <atomic>

#include "../util/precise_sleep.hpp"

namespace NUClear {
namespace extension {

    ChronoController::ChronoController(std::unique_ptr<NUClear::Environment> environment)
        : Reactor(std::move(environment)) {

        // Estimate the accuracy of our cv wait and precise sleep
        for (int i = 0; i < 3; ++i) {
            // Estimate the accuracy of our cv wait
            std::mutex test;
            std::unique_lock<std::mutex> lock(test);
            const auto cv_s = NUClear::clock::now();
            wait.wait_for(lock, std::chrono::milliseconds(1));
            const auto cv_e = NUClear::clock::now();
            const auto cv_a = NUClear::clock::duration(cv_e - cv_s - std::chrono::milliseconds(1));

            // Estimate the accuracy of our precise sleep
            const auto ns_s = NUClear::clock::now();
            util::precise_sleep(std::chrono::milliseconds(1));
            const auto ns_e = NUClear::clock::now();
            const auto ns_a = NUClear::clock::duration(ns_e - ns_s - std::chrono::milliseconds(1));

            // Use the largest time we have seen
            cv_accuracy = cv_a > cv_accuracy ? cv_a : cv_accuracy;
            ns_accuracy = ns_a > ns_accuracy ? ns_a : ns_accuracy;
        }

        on<Trigger<ChronoTask>>().then("Add Chrono task", [this](const std::shared_ptr<const ChronoTask>& task) {
            // Lock the mutex while we're doing stuff
            const std::lock_guard<std::mutex> lock(mutex);

            // Add our new task to the heap if we are still running
            if (running.load(std::memory_order_acquire)) {
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
            running.store(false, std::memory_order_release);
            const std::lock_guard<std::mutex> lock(mutex);
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
            while (running.load(std::memory_order_acquire)) {

                // Acquire the mutex lock so we can wait on it
                std::unique_lock<std::mutex> lock(mutex);

                // If we have no chrono tasks wait until we are notified
                if (tasks.empty()) {
                    wait.wait(lock, [this] { return !running.load(std::memory_order_acquire) || !tasks.empty(); });
                }
                else {
                    auto start  = NUClear::clock::now();
                    auto target = tasks.front().time;

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
                    else {
                        const NUClear::clock::duration time_until_task =
                            std::chrono::duration_cast<NUClear::clock::duration>((target - start) / clock::rtf());

                        if (clock::rtf() == 0.0) {
                            // If we are paused then just wait until we are unpaused
                            wait.wait(lock, [&] {
                                return !running.load(std::memory_order_acquire) || clock::rtf() != 0.0
                                       || NUClear::clock::now() != start;
                            });
                        }
                        else if (time_until_task > cv_accuracy) {  // A long time in the future
                            // Wait on the cv
                            wait.wait_for(lock, time_until_task - cv_accuracy);

                            // Update the accuracy of our cv wait
                            const auto end   = NUClear::clock::now();
                            const auto error = end - (target - cv_accuracy);  // when ended - when wanted to end
                            if (error.count() > 0) {                          // only if we were late
                                cv_accuracy = error > cv_accuracy ? error : ((cv_accuracy * 99 + error) / 100);
                            }
                        }
                        else if (time_until_task > ns_accuracy) {  // Somewhat close in time
                            // Wait on nanosleep
                            const NUClear::clock::duration sleep_time = time_until_task - ns_accuracy;
                            util::precise_sleep(sleep_time);

                            // Update the accuracy of our precise sleep
                            const auto end   = NUClear::clock::now();
                            const auto error = end - (target - ns_accuracy);  // when ended - when wanted to end
                            if (error.count() > 0) {                          // only if we were late
                                ns_accuracy = error > ns_accuracy ? error : ((ns_accuracy * 99 + error) / 100);
                            }
                        }
                        else {
                            while (NUClear::clock::now() < tasks.front().time) {
                                // Spinlock until we get to the time
                            }
                        }
                    }
                }
            }
        });
    }

}  // namespace extension
}  // namespace NUClear
