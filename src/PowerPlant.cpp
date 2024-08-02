/*
 * MIT License
 *
 * Copyright (c) 2013 NUClear Contributors
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

#include "PowerPlant.hpp"

#include <exception>
#include <tuple>

#include "Reactor.hpp"
#include "dsl/store/DataStore.hpp"
#include "dsl/word/Shutdown.hpp"
#include "dsl/word/Startup.hpp"
#include "dsl/word/emit/Direct.hpp"
#include "message/CommandLineArguments.hpp"
#include "message/LogMessage.hpp"
#include "threading/ReactionTask.hpp"

namespace NUClear {
namespace util {
    struct GroupDescriptor;
    struct ThreadPoolDescriptor;
}  // namespace util

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
PowerPlant* PowerPlant::powerplant = nullptr;

PowerPlant::PowerPlant(Configuration config, int argc, const char* argv[]) : scheduler(config.thread_count) {

    // Stop people from making more then one powerplant
    if (powerplant != nullptr) {
        throw std::runtime_error("There is already a powerplant in existence (There should be a single PowerPlant)");
    }

    // Store our static variable
    powerplant = this;

    // Emit our arguments if any.
    message::CommandLineArguments args;
    for (int i = 0; i < argc; ++i) {
        args.emplace_back(argv[i]);
    }

    // Emit our command line arguments
    emit(std::make_unique<message::CommandLineArguments>(args));
}

PowerPlant::~PowerPlant() {
    // Make sure reactors are destroyed before anything else
    while (!reactors.empty()) {
        reactors.pop_back();
    }

    // Bye bye powerplant
    powerplant = nullptr;
}

void PowerPlant::start() {

    // We are now running
    is_running.store(true);

    // Direct emit startup event and command line arguments
    emit<dsl::word::emit::Direct>(std::make_unique<dsl::word::Startup>());
    emit_shared<dsl::word::emit::Direct>(dsl::store::DataStore<message::CommandLineArguments>::get());

    // Start all of the threads
    scheduler.start();
}

void PowerPlant::submit(std::unique_ptr<threading::ReactionTask>&& task, const bool& immediate) noexcept {
    // Only submit non null tasks
    if (task) {
        scheduler.submit(std::move(task), immediate);
    }
}

void PowerPlant::log(const LogLevel& level, const std::string& message) {
    // Get the current task
    const auto* current_task = threading::ReactionTask::get_current_task();

    // Direct emit the log message so that any direct loggers can use it
    emit<dsl::word::emit::Direct>(std::make_unique<message::LogMessage>(
        level,
        current_task != nullptr ? current_task->parent->reactor.log_level : LogLevel::UNKNOWN,
        message,
        current_task != nullptr ? current_task->stats : nullptr));
}
void PowerPlant::log(const LogLevel& level, std::stringstream& message) {
    log(level, message.str());
}

void PowerPlant::add_idle_task(const NUClear::id_t& id,
                               const util::ThreadPoolDescriptor& pool_descriptor,
                               std::function<void()>&& task) {
    scheduler.add_idle_task(id, pool_descriptor, std::move(task));
}

void PowerPlant::remove_idle_task(const NUClear::id_t& id, const util::ThreadPoolDescriptor& pool_descriptor) {
    scheduler.remove_idle_task(id, pool_descriptor);
}

void PowerPlant::shutdown() {

    // Stop running before we emit the Shutdown event
    // Some things such as on<Always> depend on this flag and it's possible to miss it
    is_running.store(false);

    // Emit our shutdown event
    emit(std::make_unique<dsl::word::Shutdown>());

    // Shutdown the scheduler
    scheduler.shutdown();
}

bool PowerPlant::running() const {
    return is_running.load();
}

}  // namespace NUClear
