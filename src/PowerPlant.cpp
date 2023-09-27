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

namespace NUClear {

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
PowerPlant* PowerPlant::powerplant = nullptr;

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

void PowerPlant::submit(const NUClear::id_t& id,
                        const int& priority,
                        const util::GroupDescriptor& group,
                        const util::ThreadPoolDescriptor& pool,
                        const bool& immediate,
                        std::function<void()>&& task) {
    scheduler.submit(id, priority, group, pool, immediate, std::move(task));
}

void PowerPlant::submit(std::unique_ptr<threading::ReactionTask>&& task, const bool& immediate) noexcept {
    // Only submit non null tasks
    if (task) {
        try {
            const std::shared_ptr<threading::ReactionTask> t(std::move(task));
            submit(t->id, t->priority, t->group_descriptor, t->thread_pool_descriptor, immediate, [t]() { t->run(); });
        }
        catch (const std::exception& ex) {
            task->parent.reactor.log<NUClear::ERROR>("There was an exception while submitting a reaction", ex.what());
        }
        catch (...) {
            task->parent.reactor.log<NUClear::ERROR>("There was an unknown exception while submitting a reaction");
        }
    }
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
