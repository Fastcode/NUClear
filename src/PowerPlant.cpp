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

#include "PowerPlant.hpp"

#include "threading/ThreadPoolTask.hpp"

namespace NUClear {

PowerPlant* PowerPlant::powerplant = nullptr;  // NOLINT

PowerPlant::~PowerPlant() {

    // Bye bye powerplant
    powerplant = nullptr;
}

void PowerPlant::on_startup(std::function<void()>&& func) {
    if (is_running) { throw std::runtime_error("Unable to do on_startup as the PowerPlant has already started"); }
    else {
        startup_tasks.push_back(func);
    }
}

void PowerPlant::add_thread_task(std::function<void()>&& task) {
    tasks.push_back(task);
}

void PowerPlant::start() {

    // We are now running
    is_running = true;

    // Run all our Initialise scope tasks
    for (auto&& func : startup_tasks) {
        func();
    }
    startup_tasks.clear();

    // Direct emit startup event
    emit<dsl::word::emit::Direct>(std::make_unique<dsl::word::Startup>());

    // Start all our threads
    for (size_t i = 0; i < configuration.thread_count; ++i) {
        tasks.push_back(threading::make_thread_pool_task(scheduler));
    }

    // Start all our tasks
    for (auto& task : tasks) {
        threads.push_back(std::make_unique<std::thread>(task));
    }

    // Start our main thread using our main task scheduler
    threading::make_thread_pool_task(main_thread_scheduler)();

    // Now wait for all the threads to finish executing
    for (auto& thread : threads) {
        try {
            if (thread->joinable()) { thread->join(); }
        }
        // This gets thrown some time if between checking if joinable and joining
        // the thread is no longer joinable
        catch (const std::system_error&) {
        }
    }
}

void PowerPlant::submit(std::unique_ptr<threading::ReactionTask>&& task) {
    scheduler.submit(std::move(task));
}

void PowerPlant::submit_main(std::unique_ptr<threading::ReactionTask>&& task) {
    main_thread_scheduler.submit(std::move(task));
}

void PowerPlant::shutdown() {

    // Stop running before we emit events the Shutdown event
    // Some things such as on<Always> depend on this flag and it's possible to miss it
    is_running = false;

    // Emit our shutdown event
    emit(std::make_unique<dsl::word::Shutdown>());

    // Shutdown the scheduler
    scheduler.shutdown();

    // Shutdown the main threads scheduler
    main_thread_scheduler.shutdown();

    // Bye bye powerplant
    powerplant = nullptr;
}

bool PowerPlant::running() const {
    return is_running;
}
}  // namespace NUClear
