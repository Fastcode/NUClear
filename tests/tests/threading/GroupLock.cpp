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
#include "threading/scheduler/GroupLock.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <list>

#include "threading/scheduler/IdleLock.hpp"
#include "threading/scheduler/Pool.hpp"
#include "util/GroupDescriptor.hpp"
#include "util/ThreadPoolDescriptor.hpp"

namespace NUClear {
namespace threading {
    namespace scheduler {

        using util::GroupDescriptor;
        using util::ThreadPoolDescriptor;

        void check_locks(const std::list<std::unique_ptr<Lock>>& locks, const std::vector<bool>& expected) {
            std::vector<bool> states;
            for (const auto& l : locks) {
                states.push_back(l->lock());
            }
            CHECK(states == expected);
        }

        SCENARIO("Locking returns false when all tokens are used") {

            GIVEN("A group with 2 tokens") {
                auto group = std::make_shared<Group>(GroupDescriptor{0, 2});

                WHEN("Two locks are created") {
                    auto lock1 = std::make_unique<GroupLock>(group);
                    auto lock2 = std::make_unique<GroupLock>(group);

                    THEN("Both locks should be able to lock") {
                        CHECK(lock1->lock());
                        CHECK(lock2->lock());

                        AND_WHEN("A third lock is created") {
                            auto lock3 = std::make_unique<GroupLock>(group);

                            THEN("The third lock should not be able to lock") {
                                CHECK_FALSE(lock3->lock());
                            }
                        }
                    }
                }
            }
        }

        SCENARIO("Unlocking allows other locks to lock") {

            GIVEN("A group with 1 token") {
                auto group = std::make_shared<Group>(GroupDescriptor{0, 1});

                WHEN("Two locks are created") {
                    auto lock1 = std::make_unique<GroupLock>(group);
                    auto lock2 = std::make_unique<GroupLock>(group);

                    THEN("Only the first should be able to lock") {
                        CHECK(lock1->lock());
                        CHECK_FALSE(lock2->lock());

                        AND_WHEN("The first lock is released") {
                            lock1.reset();

                            THEN("The second lock should be able to lock") {
                                CHECK(lock2->lock());
                            }
                        }
                    }
                }
            }
        }

        SCENARIO("Releasing locks notifies appropriate waiting locks") {

            GIVEN("A group with 1 token") {
                auto group = std::make_shared<Group>(GroupDescriptor{0, 1});

                WHEN("Three locks are created") {
                    std::vector<int> notifications;
                    auto lock1 = std::make_unique<GroupLock>(group, [&] { notifications.push_back(1); });
                    auto lock2 = std::make_unique<GroupLock>(group, [&] { notifications.push_back(2); });
                    auto lock3 = std::make_unique<GroupLock>(group, [&] { notifications.push_back(3); });

                    THEN("Only the first should be able to lock") {
                        CHECK(lock1->lock());
                        CHECK_FALSE(lock2->lock());
                        CHECK_FALSE(lock3->lock());

                        AND_WHEN("The last unlocked lock is released") {
                            lock3.reset();

                            THEN("No notifications should have been made") {
                                CHECK(notifications.empty());
                            }
                        }

                        AND_WHEN("The locked lock is released") {
                            lock1.reset();

                            THEN("The two waiting locks should be notified") {
                                CHECK(notifications == std::vector<int>{2, 3});
                            }
                        }
                    }
                }
            }
        }

        SCENARIO("Notifications only occur once for each lock") {

            GIVEN("A group with 1 token") {
                auto group = std::make_shared<Group>(GroupDescriptor{0, 1});

                WHEN("Two locks are created") {
                    std::vector<int> notifications;
                    auto lock1 = std::make_unique<GroupLock>(group, [&] { notifications.push_back(1); });
                    auto lock2 = std::make_unique<GroupLock>(group, [&] { notifications.push_back(2); });

                    THEN("Only the first should be able to lock") {
                        for (int i = 0; i < 3; ++i) {
                            CHECK(lock1->lock());
                            CHECK_FALSE(lock2->lock());
                        }

                        AND_WHEN("The first lock is released") {
                            lock1.reset();

                            THEN("The second lock should be notified once") {
                                CHECK(notifications == std::vector<int>{2});
                            }
                        }
                    }
                }
            }
        }

        SCENARIO("Locks which are deleted should not notify") {

            GIVEN("A group with 1 token") {
                auto group = std::make_shared<Group>(GroupDescriptor{0, 1});

                WHEN("Three locks are created") {
                    std::vector<int> notifications;
                    auto lock1 = std::make_unique<GroupLock>(group, [&] { notifications.push_back(1); });
                    auto lock2 = std::make_unique<GroupLock>(group, [&] { notifications.push_back(2); });
                    auto lock3 = std::make_unique<GroupLock>(group, [&] { notifications.push_back(3); });

                    THEN("Only the first should be able to lock") {
                        CHECK(lock1->lock());
                        CHECK_FALSE(lock2->lock());
                        CHECK_FALSE(lock3->lock());

                        AND_WHEN("The second and then first lock is released") {
                            lock2.reset();
                            lock1.reset();

                            THEN("Only the third lock should be notified once") {
                                CHECK(notifications == std::vector<int>{3});
                            }
                        }
                    }
                }
            }
        }

    }  // namespace scheduler
}  // namespace threading
}  // namespace NUClear
