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

    void ChronoController::add(const std::shared_ptr<dsl::operation::ChronoTask>& task) {
        const std::lock_guard<std::mutex> lock(mutex);

        // Add our new task to the heap if we are still running
        if (running.load(std::memory_order_acquire)) {
            tasks.push_back(task);
            std::push_heap(tasks.begin(), tasks.end(), std::greater<>());
        }
    }

    void ChronoController::remove(const NUClear::id_t& id) {
        const std::lock_guard<std::mutex> lock(mutex);

        // Find the task
        auto it = std::find_if(tasks.begin(), tasks.end(), [&](const auto& task) { return task->id == id; });

        // Remove if it exists
        if (it != tasks.end()) {
            tasks.erase(it);
            std::make_heap(tasks.begin(), tasks.end(), std::greater<>());
        }

        // Poke the system to make sure it's not waiting on something that's gone
        sleeper.wake();
    }

    NUClear::clock::time_point ChronoController::next() {
        const std::lock_guard<std::mutex> lock(mutex);

        // If we have no tasks return a nullptr
        if (tasks.empty()) {
            return NUClear::clock::time_point::max();
        }

        auto target = tasks.front()->time;
        auto now    = NUClear::clock::now();

        // Run the task if we are at or past the target time
        if (target <= now) {
            auto task  = tasks.front();
            bool renew = task->run();
            std::pop_heap(tasks.begin(), tasks.end(), std::greater<>());

            if (renew) {
                std::push_heap(tasks.begin(), tasks.end(), std::greater<>());
            }
            else {
                tasks.pop_back();
            }
        }

        return target;
    }

    ChronoController::ChronoController(std::unique_ptr<NUClear::Environment> environment)
        : Reactor(std::move(environment)) {

        on<Trigger<ChronoTask>>().then("Add Chrono task", [this](const std::shared_ptr<const ChronoTask>& task) {
            add(std::const_pointer_cast<ChronoTask>(task));
            sleeper.wake();
        });

        on<Trigger<Unbind>>().then("Unbind Chrono Task", [this](const Unbind& unbind) {
            remove(unbind.id);
            sleeper.wake();
        });

        // When we shutdown we notify so we quit now
        on<Shutdown>().then("Shutdown Chrono Controller", [this] {
            running.store(false, std::memory_order_release);
            sleeper.wake();
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
                        task->time += adjustment;
                    }

                } break;
                case message::TimeTravel::Action::NEAREST: {
                    const clock::time_point nearest =
                        tasks.empty() ? travel.target
                                      : std::min(travel.target, (*std::min_element(tasks.begin(), tasks.end()))->time);
                    clock::set_clock(nearest, travel.rtf);
                } break;
            }

            // Poke the system
            sleeper.wake();
        });

        on<Always, Priority::REALTIME>().then("Chrono Controller", [this] {
            // Run until we are told to stop
            while (running) {

                // Run the next task and get the target time to wait until
                auto target = next();

                // Wait until the next task or we are woken
                auto now = NUClear::clock::now();
                if (target > now) {
                    // Calculate the real time to sleep given the rate at which time passes
                    NUClear::clock::duration nuclear_sleep_time = target - now;
                    const auto time_until_task =
                        clock::rtf() == 0.0 ? std::chrono::steady_clock::time_point::max()
                                            : std::chrono::steady_clock::now() + ns(nuclear_sleep_time / clock::rtf());

                    sleeper.sleep_until(time_until_task);
                }
            }
        });
    }

}  // namespace extension
}  // namespace NUClear
