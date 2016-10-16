/*
 * Copyright (C) 2013-2016 Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
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

#include "nuclear_bits/clock.hpp"

namespace NUClear {
    namespace dsl {
        namespace operation {

            struct ChronoTask {
                
                ChronoTask(std::function<bool(NUClear::clock::time_point&)>&& task, NUClear::clock::time_point time, uint64_t id)
                : task(task)
                , time(time)
                , id(id) {
                }
                
                inline bool operator() () {
                    return task(time);
                }
                
                inline bool operator< (const ChronoTask& other) const {
                    return time > other.time;
                }
                
                std::function<bool(NUClear::clock::time_point&)> task;
                NUClear::clock::time_point time;
                uint64_t id;
            };

        }  // namespace operation
    }  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_OPERATION_TYPEBIND_HPP
