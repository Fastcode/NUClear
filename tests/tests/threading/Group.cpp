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
#include <atomic>
#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <chrono>
#include <cstdint>
#include <memory>
#include <random>
#include <set>
#include <thread>
#include <utility>
#include <vector>

#include "id.hpp"
// Group's WaitEntry holds a std::unique_ptr<ReactionTask>, so a complete type is needed at the
// point where TaskQueue<WaitEntry> is instantiated (which happens via Group's constructor).
#include "threading/ReactionTask.hpp"  // NOLINT(misc-include-cleaner)
#include "threading/scheduler/Lock.hpp"
#include "threading/scheduler/Pool.hpp"
#include "threading/scheduler/Scheduler.hpp"
#include "util/GroupDescriptor.hpp"
#include "util/Inline.hpp"
#include "util/ThreadPoolDescriptor.hpp"

namespace NUClear {
namespace threading {
    namespace scheduler {

        namespace {
            std::shared_ptr<Group> make_group(int n_tokens) {
                auto desc = std::make_shared<util::GroupDescriptor>("Test", n_tokens);
                return std::make_shared<Group>(desc);
            }

            std::unique_ptr<ReactionTask> make_test_task(const int priority = 1) {
                return std::make_unique<ReactionTask>(
                    nullptr,
                    false,
                    [priority](const ReactionTask& /*task*/) { return priority; },
                    [](const ReactionTask& /*task*/) { return util::Inline::NEVER; },
                    [](const ReactionTask& /*task*/) {
                        return std::make_shared<util::ThreadPoolDescriptor>("TestPool", 1);
                    },
                    [](const ReactionTask& /*task*/) {
                        return std::set<std::shared_ptr<const util::GroupDescriptor>>{};
                    });
            }

            /// A ReactionTask that bumps a completion counter when run by a pool worker.
            std::unique_ptr<ReactionTask> make_counting_task(std::atomic<int>& completed, const int priority = 1) {
                auto task      = make_test_task(priority);
                task->callback = [&completed](ReactionTask& /*task*/) {
                    completed.fetch_add(1, std::memory_order_acq_rel);
                };
                return task;
            }

            /// Spin (with a small back-off) until `pred()` is true or `timeout` elapses.
            /// Returns the final value of `pred()` so callers can assert-rather-than-hang.
            template <typename Pred>
            bool wait_for(Pred&& pred, const std::chrono::milliseconds timeout) {
                const auto deadline = std::chrono::steady_clock::now() + timeout;
                while (std::chrono::steady_clock::now() < deadline) {
                    if (pred()) {
                        return true;
                    }
                    std::this_thread::sleep_for(std::chrono::microseconds(100));
                }
                return pred();
            }

            /// Repeatedly attempt to acquire a slow-path lock until it succeeds or `timeout` elapses.
            bool acquire_blocking(Lock& lock, const std::chrono::milliseconds timeout) {
                const auto deadline = std::chrono::steady_clock::now() + timeout;
                while (!lock.lock()) {
                    if (std::chrono::steady_clock::now() >= deadline) {
                        return false;
                    }
                    std::this_thread::sleep_for(std::chrono::microseconds(50));
                }
                return true;
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

        SCENARIO("Opportunistic drain during park publish must not leak group tokens") {
            GIVEN("A group with one token and a slow-path holder") {
                auto group                   = make_group(1);
                NUClear::id_t task_id_source = 0;

                Group::TestAccess::set_capture_drains(*group, true);

                std::unique_ptr<Lock> slow_lock = group->lock(++task_id_source, 1, [] {});
                CHECK(slow_lock->lock() == true);

                WHEN("A fast waiter publishes, the slow lock releases, then the waiter reconciles") {
                    auto slot = Group::TestAccess::park_publish(*group, make_test_task(), nullptr, false);

                    slow_lock.reset();

                    Group::TestAccess::park_reconcile(*group, slot);

                    THEN("All tokens are restored after quiescing and the group is not deadlocked") {
                        auto captured = Group::TestAccess::take_captured_drains(*group);
                        REQUIRE(captured.size() == 1);
                        captured.front().lock.reset();

                        CHECK(Group::TestAccess::tokens(*group) == group->descriptor->concurrency);
                        CHECK(Group::TestAccess::try_acquire_running_lock(*group) != nullptr);
                    }
                }
            }
        }

        SCENARIO("Concurrent fast and slow path traffic never leaks group tokens or deadlocks") {
            const int concurrency = GENERATE(1, 2, 3);
            CAPTURE(concurrency);

            GIVEN("Two groups served by a started worker pool") {
                // A real worker pool so drained/submitted tasks actually run and release their
                // group tokens. counts_for_idle=false keeps the idle machinery out of this focused
                // token-accounting test.
                auto scheduler = std::make_unique<Scheduler>(4);
                auto pool_desc =
                    std::make_shared<util::ThreadPoolDescriptor>("GroupStressPool", 4, /*counts_for_idle=*/false);
                auto pool = std::make_unique<Pool>(*scheduler, pool_desc);
                pool->start();

                std::array<std::shared_ptr<Group>, 2> groups{make_group(concurrency), make_group(concurrency)};

                std::atomic<int> submitted{0};
                std::atomic<int> completed{0};
                std::atomic<bool> stop{false};
                std::atomic<bool> burst{false};
                std::atomic<bool> acquire_failed{false};

                WHEN("A holder repeatedly grabs both groups while fast submits flood and park") {
                    // Round structure (repeated many times to randomise the interleaving):
                    //   1. The holder grabs BOTH groups via the slow path while traffic is quiet, so
                    //      the multi-group acquire never starves (the real scheduler makes a
                    //      multi-group lock wait for genuine availability, which cannot be satisfied
                    //      under a continuous single-group flood).
                    //   2. It flips `burst` on; the fast threads then pour single-group submits in,
                    //      all of which park because slow_pending > 0 while the holder holds.
                    //   3. It releases both groups. Each release runs the GroupLock opportunistic
                    //      drain, racing it against fast submits that are still mid publish/reconcile.
                    // This hammers exactly the window from concern #1 across hundreds of rounds per
                    // concurrency level (and many more across repeated binary/TSAN runs).
                    constexpr int n_fast_threads = 4;
                    constexpr int rounds         = 200;
                    constexpr int burst_target   = 3;

                    std::vector<std::thread> fast_threads;
                    for (int t = 0; t < n_fast_threads; ++t) {
                        fast_threads.emplace_back([&, t] {
                            std::mt19937 rng(0x51EDU + static_cast<unsigned>(t));
                            bool submitted_this_burst = false;
                            while (!stop.load(std::memory_order_acquire)) {
                                if (burst.load(std::memory_order_acquire)) {
                                    if (!submitted_this_burst) {
                                        auto& g = groups[rng() & 1U];
                                        submitted.fetch_add(1, std::memory_order_acq_rel);
                                        g->try_submit(make_counting_task(completed), pool.get(), false);
                                        submitted_this_burst = true;
                                    }
                                    std::this_thread::yield();
                                }
                                else {
                                    submitted_this_burst = false;
                                    std::this_thread::sleep_for(std::chrono::microseconds(20));
                                }
                            }
                        });
                    }

                    for (int r = 0; r < rounds; ++r) {
                        const NUClear::id_t id = ReactionTask::next_id();

                        // Acquire both groups in a fixed order while quiet (no burst in flight).
                        auto lock0      = groups[0]->lock(id, 1, [] {});
                        const bool got0 = acquire_blocking(*lock0, std::chrono::seconds(10));
                        auto lock1      = groups[1]->lock(id, 1, [] {});
                        const bool got1 = got0 && acquire_blocking(*lock1, std::chrono::seconds(10));
                        if (!got0 || !got1) {
                            acquire_failed.store(true, std::memory_order_release);
                        }

                        // Flood: fast submits now park behind the held groups.
                        const int before = submitted.load(std::memory_order_acquire);
                        burst.store(true, std::memory_order_release);
                        wait_for(
                            [&] {
                                return submitted.load(std::memory_order_acquire) - before >= burst_target;
                            },
                            std::chrono::seconds(2));
                        burst.store(false, std::memory_order_release);

                        // Release in reverse order; each dtor hands its freed token to a parked waiter,
                        // racing that drain against fast submits still mid publish/reconcile.
                        lock1.reset();
                        lock0.reset();

                        // Let this round's parked tasks fully drain (slow_pending is 0 now) before the
                        // next acquire. Without this the next round's lock() re-raises slow_pending and
                        // legitimately defers not-yet-drained fast waiters (slow path has priority);
                        // that is expected scheduler behaviour, not a leak.
                        const bool quiesced = wait_for(
                            [&] {
                                return Group::TestAccess::tokens(*groups[0]) == concurrency
                                       && Group::TestAccess::tokens(*groups[1]) == concurrency;
                            },
                            std::chrono::seconds(10));
                        REQUIRE(quiesced);
                    }

                    stop.store(true, std::memory_order_release);
                    for (auto& th : fast_threads) {
                        th.join();
                    }

                    // (a) NO DEADLOCK: every submitted task must eventually run. Bounded wait so a
                    // leaked token surfaces as a test failure instead of an indefinite hang.
                    const bool all_ran = wait_for(
                        [&] {
                            return completed.load(std::memory_order_acquire)
                                   == submitted.load(std::memory_order_acquire);
                        },
                        std::chrono::seconds(30));

                    THEN("Every task runs, tokens return to concurrency, and fresh submits schedule") {
                        CHECK_FALSE(acquire_failed.load(std::memory_order_acquire));
                        REQUIRE(all_ran);

                        // (b) No leaked/duplicated tokens, and the group is still usable.
                        for (auto& g : groups) {
                            CHECK(Group::TestAccess::tokens(*g) == concurrency);
                            auto fresh = Group::TestAccess::try_acquire_running_lock(*g);
                            CHECK(fresh != nullptr);
                            fresh.reset();
                            CHECK(Group::TestAccess::tokens(*g) == concurrency);
                        }
                    }

                    // Tear down cleanly when healthy; on failure leak the pool/scheduler so a
                    // genuinely deadlocked run reports the failed assertion instead of hanging join.
                    if (all_ran) {
                        pool->stop(Pool::StopType::FORCE);
                        pool->join();
                    }
                    else {
                        (void) pool.release();
                        (void) scheduler.release();
                    }
                }
            }
        }

    }  // namespace scheduler
}  // namespace threading
}  // namespace NUClear
