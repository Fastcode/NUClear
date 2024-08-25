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
#include "extension/ChronoController.hpp"
#include "extension/IOController.hpp"
#include "extension/NetworkController.hpp"
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

// This is taking argc and argv as given by main so this should not take an array
// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
PowerPlant::PowerPlant(Configuration config, int argc, const char* argv[]) : scheduler(config.thread_count) {

    // Stop people from making more then one powerplant
    if (powerplant != nullptr) {
        throw std::runtime_error("There is already a powerplant in existence (There should be a single PowerPlant)");
    }

    // Store our static variable
    powerplant = this;

    // Install the extension controllers
    install<extension::ChronoController>();
    install<extension::IOController>();
    install<extension::NetworkController>();

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

    // Direct emit startup event and command line arguments
    emit<dsl::word::emit::Direct>(std::make_unique<dsl::word::Startup>());
    emit_shared<dsl::word::emit::Direct>(dsl::store::DataStore<message::CommandLineArguments>::get());

    // Start all of the threads
    scheduler.start();
}

void PowerPlant::add_idle_task(const std::shared_ptr<threading::Reaction>& reaction,
                               const std::shared_ptr<const util::ThreadPoolDescriptor>& pool_descriptor) {
    scheduler.add_idle_task(reaction, pool_descriptor);
}

void PowerPlant::remove_idle_task(const NUClear::id_t& id,
                                  const std::shared_ptr<const util::ThreadPoolDescriptor>& pool_descriptor) {
    scheduler.remove_idle_task(id, pool_descriptor);
}

void PowerPlant::submit(std::unique_ptr<threading::ReactionTask>&& task, const bool& immediate) noexcept {
    scheduler.submit(std::move(task), immediate);
}

void PowerPlant::log(const LogLevel& level, std::string message) {
    // Get the current task
    const auto* current_task = threading::ReactionTask::get_current_task();

    // Direct emit the log message so that any direct loggers can use it
    emit<dsl::word::emit::Direct>(std::make_unique<message::LogMessage>(
        level,
        current_task != nullptr ? current_task->parent->reactor.log_level : LogLevel::UNKNOWN,
        std::move(message),
        current_task != nullptr ? current_task->stats : nullptr));
}
void PowerPlant::log(const LogLevel& level, std::stringstream& message) {
    log(level, message.str());
}

void PowerPlant::shutdown(bool force) {

    // Emit our shutdown event
    emit(std::make_unique<dsl::word::Shutdown>());

    // Shutdown the scheduler
    scheduler.stop(force);
}

}  // namespace NUClear
