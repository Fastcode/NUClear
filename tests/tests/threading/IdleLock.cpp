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
#include "threading/scheduler/IdleLock.hpp"

#include <catch2/catch_test_macros.hpp>

namespace NUClear {
namespace threading {
    namespace scheduler {

        /**
         * Converts the status of an IdleLockPair into a character for easier comparison
         *
         * @param lock The lock to check
         * @return The character representing the lock status
         */
        char lock_status(const std::unique_ptr<IdleLockPair>& lock) {
            switch (lock->lock() << 2 | lock->local_lock() << 1 | lock->global_lock()) {
                case 0b000: return 'U';  // Unlocked
                case 0b101: return 'G';  // Global only
                case 0b110: return 'L';  // Local only
                case 0b111: return 'B';  // Both locked
                case 0b001: return 'g';  // Error: global without being locked
                case 0b010: return 'l';  // Error: local without being locked
                case 0b011: return 'b';  // Error: both without being locked
                case 0b100: return 'u';  // Error: locked without a reason
                default: return '?';
            }
        }

        SCENARIO("The last thread to lock an IdleLock should obtain the lock until releasing it",
                 "[threading][scheduler][IdleLock]") {
            GIVEN("A semaphore with a value of 2") {
                std::atomic<IdleLock::semaphore_t> active{2};

                WHEN("Two locks are attempted") {
                    std::unique_ptr<Lock> a1 = std::make_unique<IdleLock>(active);
                    std::unique_ptr<Lock> a2 = std::make_unique<IdleLock>(active);

                    THEN("The last lock should obtain the lock") {
                        CHECK(a1->lock() == false);
                        CHECK(a2->lock() == true);
                    }

                    AND_WHEN("The locked lock is released and a third lock is attempted") {
                        a2.reset();
                        std::unique_ptr<Lock> a3 = std::make_unique<IdleLock>(active);

                        THEN("Only the third lock should obtain the lock") {
                            CHECK(a1->lock() == false);
                            CHECK(a3->lock() == true);
                        }
                    }

                    AND_WHEN("The unlocked lock is released and a third lock is attempted") {
                        a1.reset();
                        std::unique_ptr<Lock> a3 = std::make_unique<IdleLock>(active);

                        THEN("Only the originally locked lock should obtain the lock") {
                            CHECK(a2->lock() == true);
                            CHECK(a3->lock() == false);
                        }
                    }
                }
            }
        }

        SCENARIO("IdleLockPair locks global and local separately", "[threading][scheduler][IdleLockPair]") {
            GIVEN("Two local semaphores with a value of 2 and a global semaphore") {
                std::atomic<IdleLock::semaphore_t> a{2};
                std::atomic<IdleLock::semaphore_t> b{2};
                std::atomic<IdleLock::semaphore_t> g{4};

                WHEN("A lock is attempted for each local semaphore") {
                    std::unique_ptr<IdleLockPair> a1 = std::make_unique<IdleLockPair>(a, g);
                    std::unique_ptr<IdleLockPair> b1 = std::make_unique<IdleLockPair>(b, g);

                    THEN("No locks should be obtained") {
                        CHECK(lock_status(a1) == 'U');
                        CHECK(lock_status(b1) == 'U');
                    }

                    AND_WHEN("A second lock is attempted for each local semaphore") {
                        std::unique_ptr<IdleLockPair> a2 = std::make_unique<IdleLockPair>(a, g);
                        std::unique_ptr<IdleLockPair> b2 = std::make_unique<IdleLockPair>(b, g);

                        THEN("A should obtain a local lock, and B should obtain both locks") {
                            CHECK(lock_status(a1) == 'U');
                            CHECK(lock_status(a2) == 'L');
                            CHECK(lock_status(b1) == 'U');
                            CHECK(lock_status(b2) == 'B');
                        }

                        AND_WHEN("Locking and unlocking to allow `A` to obtain only a global lock") {
                            b2.reset();
                            a1.reset();
                            b2 = std::make_unique<IdleLockPair>(b, g);
                            a1 = std::make_unique<IdleLockPair>(a, g);

                            THEN("`A` should only obtain a global lock, and `B` only a local lock") {
                                CHECK(lock_status(a1) == 'G');
                                CHECK(lock_status(a2) == 'L');
                                CHECK(lock_status(b1) == 'U');
                                CHECK(lock_status(b2) == 'L');
                            }
                        }
                    }
                }
            }
        }

    }  // namespace scheduler
}  // namespace threading
}  // namespace NUClear
