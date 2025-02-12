/*
 * MIT License
 *
 * Copyright (c) 2023 NUClear Contributors
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

#include "util/ThreadPriority.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

namespace NUClear {
namespace util {

    SCENARIO("ThreadPriority sets and restores thread priority levels") {

        PriorityLevel initial = get_current_thread_priority();
        GIVEN("A thread with initially " << initial << " priority") {

            PriorityLevel priority_1 = GENERATE(PriorityLevel::IDLE,
                                                PriorityLevel::LOWEST,
                                                PriorityLevel::LOW,
                                                PriorityLevel::NORMAL,
                                                PriorityLevel::HIGH,
                                                PriorityLevel::HIGHEST,
                                                PriorityLevel::REALTIME);
            PriorityLevel priority_2 = GENERATE(PriorityLevel::IDLE,
                                                PriorityLevel::LOWEST,
                                                PriorityLevel::LOW,
                                                PriorityLevel::NORMAL,
                                                PriorityLevel::HIGH,
                                                PriorityLevel::HIGHEST,
                                                PriorityLevel::REALTIME);

            WHEN("ThreadPriority is set to " << priority_1) {
                {
                    ThreadPriority priority_lock_1(priority_1);

                    THEN("The thread priority should be " << priority_1) {
                        REQUIRE(get_current_thread_priority() == priority_1);
                    }
                    AND_WHEN("ThreadPriority is set to " << priority_2) {
                        {
                            ThreadPriority priority_lock_2(priority_2);
                            THEN("The thread priority should be " << priority_2) {
                                REQUIRE(get_current_thread_priority() == priority_2);
                            }
                        }
                        AND_WHEN("ThreadPriority is destroyed") {
                            THEN("The thread priority should be restored to " << priority_1) {
                                REQUIRE(get_current_thread_priority() == priority_1);
                            }
                        }
                    }
                }
                AND_WHEN("ThreadPriority is destroyed") {
                    THEN("The thread priority should be restored to " << initial) {
                        REQUIRE(get_current_thread_priority() == initial);
                    }
                }
            }
        }
    }

}  // namespace util
}  // namespace NUClear
