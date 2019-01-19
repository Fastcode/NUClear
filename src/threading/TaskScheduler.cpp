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

#include "TaskScheduler.hpp"

namespace NUClear {
namespace threading {

    TaskScheduler::TaskScheduler() : running(true) {}

    void TaskScheduler::shutdown() {
        running = false;
    }

    void TaskScheduler::submit(std::unique_ptr<ReactionTask>&& task) {

        // We do not accept new tasks once we are shutdown
        if (running) {
            // TODO EMBEDDED, CRITICAL SECTION DISABLE INTERRUPTS
            queue.push(std::forward<std::unique_ptr<ReactionTask>>(task));
        }
    }

    std::unique_ptr<ReactionTask> TaskScheduler::get_task() {

        // While our queue is empty
        while (queue.empty()) {
            // If the queue is empty we either wait or shutdown

            if (!running) {
                // Return a nullptr to signify there is nothing on the queue
                return nullptr;
            }
        }

        // Return the type
        // If you're wondering why all the ridiculousness, it's because priority queue is not as feature complete as it
        // should be its 'top' method returns a const reference (which we can't use to move a unique pointer)
        // TODO embedded this should be a critical section
        std::unique_ptr<ReactionTask> task(
            std::move(const_cast<std::unique_ptr<ReactionTask>&>(queue.top())));  // NOLINT
        queue.pop();

        return task;
    }
}  // namespace threading
}  // namespace NUClear
