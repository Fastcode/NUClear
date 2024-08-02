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

#include "ReactionTask.hpp"

#include <atomic>
#include <functional>
#include <memory>
#include <utility>

#include "../id.hpp"
#include "../util/platform.hpp"
#include "Reaction.hpp"

namespace NUClear {
namespace threading {

    const ReactionTask* ReactionTask::get_current_task() {
        return current_task;
    }

    ReactionTask::~ReactionTask() {
        // Decrement the number of active tasks
        --parent->active_tasks;
    }

    void ReactionTask::run() {
        // Update the current task
        auto* t = std::exchange(current_task, this);
        // Run our callback
        callback(*this);

        // Restore the current task
        current_task = t;
    }

    NUClear::id_t ReactionTask::new_task_id() {
        static std::atomic<NUClear::id_t> task_id_source(0);
        return ++task_id_source;
    }

    // Initialize our current task
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
    ATTRIBUTE_TLS ReactionTask* ReactionTask::current_task = nullptr;

}  // namespace threading
}  // namespace NUClear
