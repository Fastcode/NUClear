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

#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "nuclear"
#include "test_util/diff_string.hpp"

// Deterministic regression test for a premature global-idle epoch, WITHOUT relying on any sleeps or
// timing windows.
//
// Topology:
//   * outer        - runs on a single-worker WorkerPool while holding the Sync group token, and
//                    dispatches the re-entrant inner task onto that same pool while STILL holding the
//                    token (so it cannot run and instead parks as an external waiter).
//   * inner        - the re-entrant task: same Sync group, same WorkerPool, LOW priority.
//   * on<Idle<>>   - the global idle reaction, also pinned to the single-worker WorkerPool, at
//                    REALTIME priority.
//
// The bug: when the WorkerPool worker is woken by the parked inner task's pending_idle latch, the
// buggy scheduler fires a global idle epoch BEFORE dequeuing the (now drained and runnable) inner
// task, dropping the pool's active count and releasing its active_pools slot while real work is
// queued.
//
// Why this is deterministic (no sleeps):
//   * Submission of the inner task is synchronous inside the outer reaction, so while the outer
//     reaction still holds the Sync token the inner task is parked (arming pending_idle) before the
//     outer reaction returns. Returning releases the token and drains the inner task into the
//     WorkerPool queue as a runnable task.
//   * The inner task and the idle reaction share the SAME single-worker WorkerPool, so they can never
//     run concurrently - the one worker serializes them.
//   * The idle reaction is REALTIME and the inner task is LOW. In the buggy case the premature idle
//     submits the idle reaction into the same queue that already holds the drained inner task; the
//     single worker then dequeues the higher-priority idle reaction FIRST, so it observes the still
//     pending inner task (awaiting_inner == true) and records the violation. In the fixed case the
//     worker dequeues the inner task first (no idle epoch), so the idle reaction never runs while the
//     inner task is pending.
//
// Result: without the fix the event sequence deterministically contains an extra
// "idle-while-inner-pending" entry; with the fix it is exactly {"outer", "inner"}.

namespace {

struct WorkerPool {
    static constexpr int concurrency = 1;
};
struct SyncGroup {};

struct Outer {};
struct Inner {};

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
std::mutex mtx;
std::vector<std::string> events;
bool awaiting_inner = false;
bool inner_done     = false;
bool shutting_down  = false;
// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

class TestReactor : public NUClear::Reactor {
public:
    explicit TestReactor(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {

        // Global idle, pinned to the (single-worker) WorkerPool at REALTIME so that if the buggy
        // scheduler submits it while the drained inner task is still queued on the same pool, it is
        // dequeued before the lower-priority inner task - making the observation deterministic.
        on<Idle<>, Pool<WorkerPool>, Priority::REALTIME>().then([this] {
            bool do_shutdown = false;
            {
                const std::lock_guard<std::mutex> lock(mtx);
                if (awaiting_inner) {
                    // Real work (the parked-then-drained inner task) is still pending on this pool, yet
                    // a global idle epoch has fired: this is the premature-idle bug.
                    events.push_back("idle-while-inner-pending");
                }
                else if (inner_done && !shutting_down) {
                    shutting_down = true;
                    do_shutdown   = true;
                }
            }
            if (do_shutdown) {
                powerplant.shutdown();
            }
        });

        // Kick off exactly one cycle.
        on<Startup>().then([this] { emit(std::make_unique<Outer>()); });

        // The outer reaction: holds the Sync token on the WorkerPool and dispatches the re-entrant
        // inner task onto that same pool while still holding that token.
        on<Trigger<Outer>, Sync<SyncGroup>, Pool<WorkerPool>, Priority::REALTIME>().then([this] {
            {
                const std::lock_guard<std::mutex> lock(mtx);
                awaiting_inner = true;
                events.push_back("outer");
            }
            // Submitted synchronously: because we still hold the Sync<SyncGroup> token, the inner task
            // fails the group lock and PARKS as an external waiter (arming the WorkerPool's
            // pending_idle latch) before this reaction returns. Returning then releases the token,
            // draining the parked inner task into the WorkerPool queue as a runnable task.
            emit(std::make_unique<Inner>());
        });

        // The re-entrant inner task: same Sync group, same WorkerPool, lower priority than idle.
        on<Trigger<Inner>, Sync<SyncGroup>, Pool<WorkerPool>, Priority::LOW>().then([this] {
            const std::lock_guard<std::mutex> lock(mtx);
            events.push_back("inner");
            awaiting_inner = false;
            inner_done     = true;
        });
    }
};

}  // namespace

TEST_CASE("Test global idle does not fire while a parked Sync waiter is pending", "[api][dsl][Idle][Pool][Sync]") {

    {
        const std::lock_guard<std::mutex> lock(mtx);
        events.clear();
        awaiting_inner = false;
        inner_done     = false;
        shutting_down  = false;
    }

    NUClear::Configuration config;
    config.default_pool_concurrency = 1;
    NUClear::PowerPlant powerplant(config);
    powerplant.install<TestReactor>();
    powerplant.start();

    const std::vector<std::string> expected = {
        "outer",
        "inner",
    };

    INFO(test_util::diff_string(expected, events));

    REQUIRE(events == expected);
}
