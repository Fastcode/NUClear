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

#ifndef NUCLEAR_DSL_OPERATION_CHRONOTASK_HPP
#define NUCLEAR_DSL_OPERATION_CHRONOTASK_HPP

#include "../../clock.hpp"

namespace NUClear {
namespace dsl {
    namespace operation {

        /**
         * @brief Emit to schedule a function to be run at a particular time.
         *
         * @details When the Chrono system is running, tasks emitted using this struct will be scheduled to execute
         *          at the given time and are passed this time as a reference. The return value from this function
         *          is used to indicate that the function should be called again in the future. If it is true the
         *          function will not be purged after execution. If the task is to be executed again it should
         *          modify the time reference to the next time that this should be executed.
         */
        struct ChronoTask {

            /**
             * @brief Constructs a new ChronoTask to execute
             *
             * @param task  the task to run, takes the time to execute as a reference so it can be updated for
             *              future runs
             * @param time  the time to execute this task
             * @param id    the unique identifer for this task
             */
            ChronoTask(std::function<bool(NUClear::clock::time_point&)>&& task,
                       NUClear::clock::time_point time,
                       uint64_t id)
                : task(task), time(time), id(id) {}

            /**
             * @brief Run the task and return true if the time has been updated to run again
             *
             * @return true if the task updated the time to run to a new time
             */
            inline bool operator()() {
                return task(time);
            }

            /**
             * @brief Compares tasks in order of soonest to execute first
             *
             * @param other the other task to compare to
             *
             * @return true if the other task is after this task
             */
            inline bool operator<(const ChronoTask& other) const {
                return time < other.time;
            }

            /**
             * @brief Compares tasks in order of soonest to execute first
             *
             * @param other the other task to compare to
             *
             * @return true if the other task is before this task
             */
            inline bool operator>(const ChronoTask& other) const {
                return time > other.time;
            }

            /**
             * @brief Check if tasks share the same execution time
             *
             * @param other the other task to compare to
             *
             * @return true if the other task is at the same time as this task
             */
            inline bool operator==(const ChronoTask& other) const {
                return time == other.time;
            }

            /// The task function, takes the time as a reference so it can be updated for multiple runs
            std::function<bool(NUClear::clock::time_point&)> task;
            /// The time this task should be executed
            NUClear::clock::time_point time;
            /// The unique identifier for this task so it can be unbound
            uint64_t id;
        };

    }  // namespace operation
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_OPERATION_TYPEBIND_HPP
