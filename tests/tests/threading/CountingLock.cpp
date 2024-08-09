/*
 * MIT License
 *
 * Copyright (c) 2024 NUClear Contributors
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
#include "threading/scheduler/CountingLock.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <memory>

namespace NUClear {
namespace threading {
    namespace scheduler {

        SCENARIO(
            "The last lock to attempt a lock which hits the target value should obtain the lock"
            "[threading][scheduler][CountingLock]") {

            const int base   = GENERATE(-1, 1, 2);
            const int offset = GENERATE(-1, 0, 1);

            GIVEN("An atomic integer with a value of 2") {
                std::atomic<int> active{2 * base + offset};

                WHEN("Two locks are attempted") {
                    std::unique_ptr<Lock> a1 = std::make_unique<CountingLock>(active, -base, offset);
                    std::unique_ptr<Lock> a2 = std::make_unique<CountingLock>(active, -base, offset);

                    THEN("The last lock should obtain the lock") {
                        CHECK(a1->lock() == false);
                        CHECK(a2->lock() == true);
                    }

                    AND_WHEN("The locked lock is released and a third lock is attempted") {
                        a2.reset();
                        std::unique_ptr<Lock> a3 = std::make_unique<CountingLock>(active, -base, offset);

                        THEN("Only the third lock should obtain the lock") {
                            CHECK(a1->lock() == false);
                            CHECK(a3->lock() == true);
                        }
                    }

                    AND_WHEN("The unlocked lock is released and a third lock is attempted") {
                        a1.reset();
                        std::unique_ptr<Lock> a3 = std::make_unique<CountingLock>(active, -base, offset);

                        THEN("The third lock should obtain the lock as well") {
                            CHECK(a2->lock() == true);
                            CHECK(a3->lock() == true);
                        }
                    }
                }
            }
        }

    }  // namespace scheduler
}  // namespace threading
}  // namespace NUClear
