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
#include "threading/scheduler/Group.hpp"

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

namespace NUClear {
namespace threading {
    namespace scheduler {

        namespace {
            std::shared_ptr<Group> make_group(int n_tokens) {
                auto desc = std::make_shared<util::GroupDescriptor>("Test", n_tokens);
                return std::make_shared<Group>(desc);
            }
        }  // namespace

        SCENARIO("When there are no tokens available the lock should be false") {
            GIVEN("A group with one token") {
                auto group                   = make_group(1);
                NUClear::id_t task_id_source = 1;

                WHEN("Creating a lock") {
                    std::unique_ptr<Lock> lock1 = group->lock(++task_id_source, 1, [] {});

                    THEN("The lock should be true") {
                        CHECK(lock1->lock() == true);
                    }

                    AND_WHEN("Creating a second lock") {
                        std::unique_ptr<Lock> lock2 = group->lock(++task_id_source, 1, [] {});

                        THEN("The lock should be false") {
                            CHECK(lock1->lock() == true);
                            CHECK(lock2->lock() == false);
                        }
                    }
                }
            }
        }

        SCENARIO("When locks are released the appropriate watchers should be notified") {
            GIVEN("A group with one token") {
                auto group                   = make_group(1);
                NUClear::id_t task_id_source = 1;

                WHEN("Creating a lock and locking it") {
                    int notified1               = 0;
                    std::unique_ptr<Lock> lock1 = group->lock(task_id_source++, 1, [&] { ++notified1; });
                    lock1->lock();

                    THEN("The lock should be true") {
                        CHECK(lock1->lock() == true);
                    }

                    AND_WHEN("Creating two more locks") {
                        int notified2               = 0;
                        int notified3               = 0;
                        std::unique_ptr<Lock> lock2 = group->lock(task_id_source++, 1, [&] { ++notified2; });
                        std::unique_ptr<Lock> lock3 = group->lock(task_id_source++, 1, [&] { ++notified3; });

                        THEN("The new locks should be false") {
                            CHECK(lock1->lock() == true);
                            CHECK(lock2->lock() == false);
                            CHECK(lock3->lock() == false);
                        }

                        AND_WHEN("The first lock is released") {
                            lock1.reset();

                            THEN("The second lock should be notified") {
                                CHECK(notified1 == 0);
                                CHECK(notified2 == 1);
                                CHECK(notified3 == 0);
                            }

                            AND_WHEN("The second lock is released") {
                                lock2.reset();

                                THEN("The third lock should be notified") {
                                    CHECK(notified1 == 0);
                                    CHECK(notified2 == 1);
                                    CHECK(notified3 == 1);
                                }
                            }
                        }
                    }
                }
            }
        }

        SCENARIO("When a higher priority task comes in it can gain a lock before a lower priority task") {
            GIVEN("A group with one token") {
                auto group = make_group(1);

                WHEN("Creating a lock and locking it") {
                    int notified1               = 0;
                    std::unique_ptr<Lock> lock1 = group->lock(1, 1, [&] { ++notified1; });

                    AND_WHEN("Locking the lock and creating a higher priority task") {
                        lock1->lock();
                        std::unique_ptr<Lock> lock2 = group->lock(2, 2, [] {});

                        THEN("The new lock should be false") {
                            CHECK(lock1->lock() == true);
                            CHECK(lock2->lock() == false);
                        }
                    }
                    AND_WHEN("Not locking the lock and creating a higher priority task") {
                        std::unique_ptr<Lock> lock2 = group->lock(2, 2, [] {});

                        THEN("The new lock should be true") {
                            CHECK(lock1->lock() == false);
                            CHECK(lock2->lock() == true);
                        }
                    }
                }
            }
        }

        SCENARIO("Tasks are locked in task/priority order regardless of input order") {
            constexpr int n_locks = 5;
            const int n_tokens    = GENERATE(1, 2);
            CAPTURE(n_tokens);

            GIVEN("A group with " << n_tokens << " tokens") {
                auto group = make_group(n_tokens);

                WHEN("Creating a series of locks out of order") {

                    std::array<int, n_locks> notified = {0, 0, 0, 0, 0};
                    std::array<std::unique_ptr<Lock>, n_locks> locks;
                    locks[3] = group->lock(3, 1, [&] { ++notified[3]; });
                    locks[1] = group->lock(1, 1, [&] { ++notified[1]; });
                    locks[4] = group->lock(4, 1, [&] { ++notified[4]; });
                    locks[0] = group->lock(0, 1, [&] { ++notified[0]; });
                    locks[2] = group->lock(2, 1, [&] { ++notified[2]; });

                    THEN("The locks should be lockable in the proper order") {
                        CHECK(locks[0]->lock() == (0 < n_tokens));
                        CHECK(locks[1]->lock() == (1 < n_tokens));
                        CHECK(locks[2]->lock() == (2 < n_tokens));
                        CHECK(locks[3]->lock() == (3 < n_tokens));
                        CHECK(locks[4]->lock() == (4 < n_tokens));

                        AND_WHEN("Releasing the locks in order") {
                            THEN("The locks are available and notified correctly") {
                                for (int i = 0; i < n_locks; ++i) {
                                    CAPTURE(i);
                                    // Release lock i
                                    CHECK(locks[i]->lock() == true);
                                    locks[i].reset();

                                    for (int j = i + 1; j < n_locks; ++j) {
                                        CAPTURE(j);
                                        CHECK(locks[j]->lock() == ((j - i - 1) < n_tokens));

                                        // The notified one would have been the one that just became lockable
                                        CHECK(notified[j] == (j == i + n_tokens ? 1 : 0));
                                    }
                                    // Reset the notified array
                                    notified.fill(0);
                                }
                            }
                        }
                    }
                }
            }
        }

        SCENARIO("Removing an unlocked lock from the queue only notifies tasks after it") {
            constexpr int n_locks = 5;

            GIVEN("A group with two tokens") {
                auto group = make_group(2);

                WHEN("Creating a series of locks") {
                    std::array<int, n_locks> notified                = {0, 0, 0, 0, 0};
                    std::array<std::unique_ptr<Lock>, n_locks> locks = {
                        group->lock(0, 1, [&] { ++notified[0]; }),
                        group->lock(1, 1, [&] { ++notified[1]; }),
                        group->lock(2, 1, [&] { ++notified[2]; }),
                        group->lock(3, 1, [&] { ++notified[3]; }),
                        group->lock(4, 1, [&] { ++notified[4]; }),
                    };

                    // Note that because this is in a scope, for the rest of the AND_WHEN calls, no locks have been
                    // acquired
                    THEN("Locking the locks should return appropriate values") {
                        CHECK(locks[0]->lock() == true);
                        CHECK(locks[1]->lock() == true);
                        CHECK(locks[2]->lock() == false);
                        CHECK(locks[3]->lock() == false);
                        CHECK(locks[4]->lock() == false);

                        AND_WHEN("Deleting the first lock") {
                            locks[0].reset();
                            THEN("The third lock should be notified") {
                                CHECK(notified[0] == 0);
                                CHECK(notified[1] == 0);
                                CHECK(notified[2] == 1);
                                CHECK(notified[3] == 0);
                                CHECK(notified[4] == 0);
                            }
                        }
                        AND_WHEN("Deleting the third lock") {
                            locks[2].reset();
                            THEN("No notifications should occur") {
                                CHECK(notified[0] == 0);
                                CHECK(notified[1] == 0);
                                CHECK(notified[2] == 0);
                                CHECK(notified[3] == 0);
                                CHECK(notified[4] == 0);
                            }
                        }
                        AND_WHEN("Deleting the first and second lock") {
                            locks[0].reset();
                            locks[1].reset();
                            THEN("The third and fourth lock should be notified once") {
                                CHECK(notified[0] == 0);
                                CHECK(notified[1] == 0);
                                CHECK(notified[2] == 1);
                                CHECK(notified[3] == 1);
                                CHECK(notified[4] == 0);
                            }
                        }
                    }
                    AND_WHEN("Not locking the locks") {
                        AND_WHEN("Deleting the first lock") {
                            locks[0].reset();
                            THEN("The second and third locks should be notified") {
                                CHECK(notified[0] == 0);
                                CHECK(notified[1] == 1);
                                CHECK(notified[2] == 1);
                                CHECK(notified[3] == 0);
                                CHECK(notified[4] == 0);
                            }
                        }
                        AND_WHEN("Deleting the third lock") {
                            locks[2].reset();
                            THEN("The first and second locks should be notified") {
                                CHECK(notified[0] == 1);
                                CHECK(notified[1] == 1);
                                CHECK(notified[2] == 0);
                                CHECK(notified[3] == 0);
                                CHECK(notified[4] == 0);
                            }
                        }
                        AND_WHEN("Locking the second lock and deleting the first lock") {
                            locks[1]->lock();
                            locks[0].reset();
                            THEN("The third lock should be notified") {
                                locks[0].reset();
                                CHECK(notified[0] == 0);
                                CHECK(notified[1] == 0);
                                CHECK(notified[2] == 1);
                                CHECK(notified[3] == 0);
                                CHECK(notified[4] == 0);
                            }
                        }
                    }
                };
            }
        }

        SCENARIO("Unlocked locks before a locked one don't interfere with notifications") {
            GIVEN("A group with two tokens") {
                auto group = make_group(2);

                WHEN("Creating a series of locks") {
                    std::array<int, 3> notified                = {0, 0, 0};
                    std::array<std::unique_ptr<Lock>, 3> locks = {
                        group->lock(0, 1, [&] { ++notified[0]; }),
                        group->lock(1, 1, [&] { ++notified[1]; }),
                        group->lock(2, 1, [&] { ++notified[2]; }),
                    };

                    THEN("Locking and then unlocking the second lock") {
                        CHECK(locks[1]->lock() == true);
                        locks[1].reset();

                        THEN("The first and third lock should be notified") {
                            CHECK(notified[0] == 1);
                            CHECK(notified[1] == 0);
                            CHECK(notified[2] == 1);
                        }
                    }
                }
            }
        }

        SCENARIO("If a lock is inserted earlier than a locked lock, it should be notified when there are spaces") {
            GIVEN("A group with one token") {
                auto group = make_group(1);

                WHEN("Creating a lock and locking it") {
                    int notified1               = 0;
                    std::unique_ptr<Lock> lock1 = group->lock(1, 1, [&] { ++notified1; });
                    lock1->lock();

                    THEN("The lock should be true") {
                        CHECK(lock1->lock() == true);
                    }

                    AND_WHEN("Creating a second lock with a higher priority") {
                        int notified2               = 0;
                        std::unique_ptr<Lock> lock2 = group->lock(2, 2, [&] { ++notified2; });

                        THEN("The new lock should be false") {
                            CHECK(lock1->lock() == true);
                            CHECK(lock2->lock() == false);
                        }

                        AND_WHEN("The first lock is released") {
                            lock1.reset();

                            THEN("The second lock should be notified and be lockable") {
                                CHECK(notified1 == 0);
                                CHECK(notified2 == 1);
                                CHECK(lock2->lock() == true);
                            }
                        }
                    }
                }
            }
        }

        SCENARIO("Locks should be notified again if they lost their priority and regained it") {
            GIVEN("A group with one token") {
                auto group = make_group(1);

                WHEN("Creating a lock and locking it") {
                    int notified1               = 0;
                    std::unique_ptr<Lock> lock1 = group->lock(1, 1, [&] { ++notified1; });
                    lock1->lock();

                    THEN("The lock should be true") {
                        CHECK(lock1->lock() == true);
                    }

                    AND_WHEN("Adding a second lock") {
                        int notified2               = 0;
                        std::unique_ptr<Lock> lock2 = group->lock(2, 1, [&] { ++notified2; });

                        THEN("The second lock should be false") {
                            CHECK(lock2->lock() == false);
                        }

                        AND_WHEN("Unlocking the first lock") {
                            lock1.reset();

                            THEN("The second lock should be notified and lockable") {
                                CHECK(notified1 == 0);
                                CHECK(notified2 == 1);
                                CHECK(lock2->lock() == true);
                            }

                            AND_WHEN("Adding a third lock with higher priority") {
                                int notified3               = 0;
                                std::unique_ptr<Lock> lock3 = group->lock(3, 2, [&] { ++notified3; });

                                THEN("The third lock should be lockable and second lock should not") {
                                    CHECK(lock3->lock() == true);
                                    CHECK(lock2->lock() == false);

                                    AND_WHEN("Unlocking the third lock") {
                                        lock3.reset();

                                        THEN("The second lock should be notified and lockable") {
                                            CHECK(notified3 == 0);
                                            CHECK(notified2 == 2);
                                            CHECK(lock2->lock() == true);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

    }  // namespace scheduler
}  // namespace threading
}  // namespace NUClear
