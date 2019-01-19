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
#include "ReactionTask.hpp"

#include <utility>

#include "Reaction.hpp"

namespace NUClear {
namespace threading {

    // Initialize our current task
    ReactionTask* ReactionTask::current_task = nullptr;

    ReactionTask::ReactionTask(Reaction& parent, int priority, TaskFunction&& callback)
        : parent(parent), priority(priority), emit_time(clock::now()), callback(callback) {}

    const ReactionTask* ReactionTask::get_current_task() {
        return current_task;
    }

    std::unique_ptr<ReactionTask> ReactionTask::run(std::unique_ptr<ReactionTask>&& us) {

        // Update our current task
        auto old_task = current_task;
        current_task  = this;

        // Run our callback at catch the returned task (to see if it rescheduled itself)
        us = callback(std::move(us));

        // Reset our task back
        current_task = old_task;

        // Return our original task
        return std::move(us);
    }
}  // namespace threading
}  // namespace NUClear
