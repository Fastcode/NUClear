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

#ifndef NUCLEAR_EXTENSION_CHRONOCONTROLLER
#define NUCLEAR_EXTENSION_CHRONOCONTROLLER

#include "../PowerPlant.hpp"
#include "../Reactor.hpp"
#include "../util/precise_sleep.hpp"

namespace NUClear {
namespace extension {

    class ChronoController : public Reactor {
    private:
        using ChronoTask = NUClear::dsl::operation::ChronoTask;

    public:
        explicit ChronoController(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {

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
                if (running.load()) {
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

            on<Always, Priority::REALTIME>().then("Chrono Controller", [this] {
                // Run until we are told to stop
                while (running.load()) {

                    // Acquire the mutex lock so we can wait on it
                    std::unique_lock<std::mutex> lock(mutex);

                    // If we have no chrono tasks wait until we are notified
                    if (tasks.empty()) {
                        wait.wait(lock, [this] { return !running.load() || !tasks.empty(); });
                    }
                    else {
                        auto start  = NUClear::clock::now();
                        auto target = tasks.front().time;

                        if (target - start > cv_accuracy) {
                            // Wait on the cv
                            wait.wait_until(lock, target - cv_accuracy);

                            // Update the accuracy of our cv wait
                            const auto end   = NUClear::clock::now();
                            const auto error = end - (target - cv_accuracy);  // when ended - when wanted to end
                            if (error.count() > 0) {                          // only if we were late
                                cv_accuracy = error > cv_accuracy ? error : ((cv_accuracy * 99 + error) / 100);
                            }
                        }
                        else if (target - start > ns_accuracy) {
                            // Wait on nanosleep
                            util::precise_sleep(target - start - ns_accuracy);

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
                    }
                }
            });
        }

    private:
        /// @brief The list of tasks we need to process
        std::vector<dsl::operation::ChronoTask> tasks;
        /// @brief The mutex we use to lock the task list
        std::mutex mutex;
        /// @brief The condition variable we use to wait on
        std::condition_variable wait;
        /// @brief If we are running or not
        std::atomic<bool> running{true};

        /// @brief The temporal accuracy when waiting on a condition variable
        NUClear::clock::duration cv_accuracy{0};
        /// @brief The temporal accuracy when waiting on nanosleep
        NUClear::clock::duration ns_accuracy{0};
    };

}  // namespace extension
}  // namespace NUClear

#endif  // NUCLEAR_EXTENSION_CHRONOCONTROLLER
