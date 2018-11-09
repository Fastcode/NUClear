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

#ifndef NUCLEAR_THREADING_THREADPOOLTASK_HPP
#define NUCLEAR_THREADING_THREADPOOLTASK_HPP

#include "../PowerPlant.hpp"
#include "../util/update_current_thread_priority.hpp"
#include "TaskScheduler.hpp"

namespace NUClear {
namespace threading {

    inline std::function<void()> make_thread_pool_task(TaskScheduler& scheduler) {
        return [&scheduler] {
            // Wait at a high (but not realtime) priority to reduce latency
            // for picking up a new task
            update_current_thread_priority(1000);

            // Run while our scheduler gives us tasks
            for (std::unique_ptr<ReactionTask> task(scheduler.get_task()); task; task = scheduler.get_task()) {

                // Run the task
                task = task->run(std::move(task));

                // Back up to realtime while waiting
                update_current_thread_priority(1000);
            }
        };
    }

}  // namespace threading
}  // namespace NUClear

#endif  // NUCLEAR_THREADING_THREADPOOLTASK_HPP
