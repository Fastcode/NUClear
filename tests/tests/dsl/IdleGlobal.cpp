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

#include <catch2/catch_test_macros.hpp>
#include <nuclear>

#include "test_util/TestBase.hpp"
#include "test_util/TimeUnit.hpp"
#include "test_util/common.hpp"
#include "util/precise_sleep.hpp"

class TestReactor : public test_util::TestBase<TestReactor> {
public:
    // A pool to use for monitoring which does not interact with idleness
    struct NonIdlePool {
        static constexpr int concurrency      = 2;
        static constexpr bool counts_for_idle = false;
    };

    explicit TestReactor(std::unique_ptr<NUClear::Environment> environment) : TestBase(std::move(environment), false) {

        /*
         * This tests that global idle only triggers one time when multiple pools go idle.
         *
         * It does this by implementing the following sequence of events.
         * Three thread pools are used MainThread (MT), Default (DT), and Idle (AT).
         * The idle pool is set up so that it doesn't interact with the Idle system so it can observe and block.
         *
         * - Start a task on DT to ensure the system is not idle
         * - Start a sync group task on AT
         * - MT should go idle
         * - Once AT is running and MT is idle, finish the task on DT
         * - DT should go idle triggering global idle
         * - This should attempt to add a task to MT but that task will be blocked by AT
         * - AT should finish, which should let the task on MT run
         * - That task should shutdown the system finishing the test
         */
        on<Startup>().then([this] { emit(std::make_unique<Step<1>>()); });

        // This is here to block other tasks from running without contributing to idleness itself
        on<Trigger<Step<1>>, Pool<NonIdlePool>, Sync<TestReactor>>().then([this] {
            // Now that we are running we hold sync
            sync_obtained.store(true, std::memory_order_release);

            // Wait for the main thread to go idle
            wait_for_set(main_idle);

            // Wait for the default thread to finish
            wait_for_set(default_idle);

            // Release sync
        });

        on<Idle<MainThread>, Pool<NonIdlePool>>().then([this] { main_idle.store(true, std::memory_order_release); });
        on<Idle<Pool<>>, Pool<NonIdlePool>>().then([this] { default_idle.store(true, std::memory_order_release); });

        on<Trigger<Step<1>>>().then([this] {
            // Wait for the main thread to be idle
            wait_for_set(main_idle);
            // Wait for always thread to start holding the pool as "not idle" until it is done
            wait_for_set(sync_obtained);
            // Finish so that the default pool can go idle
        });

        // This should happen when the default thread goes idle since we checked that the main thread is already idle
        // However it should try to run on the main thread and be blocked by the always thread therefore also idle
        on<Idle<>, MainThread, Sync<TestReactor>>().then([this] {
            idles_fired.fetch_add(1, std::memory_order_relaxed);
            emit(std::make_unique<Step<2>>());
        });

        // At low priority shutdown, this will run after all the global idles (should be 1) have been fired
        on<Trigger<Step<2>>, Priority::LOW>().then([this] { powerplant.shutdown(); });

        // This shutdown is here in case the test times out so all the spinlocks don't hang the test
        on<Shutdown, Pool<NonIdlePool>>().then([this] {
            sync_obtained.store(true, std::memory_order_release);
            main_idle.store(true, std::memory_order_release);
            default_idle.store(true, std::memory_order_release);
        });
    }

    static void wait_for_set(const std::atomic<bool>& flag) {
        while (!flag.load(std::memory_order_acquire)) {
            NUClear::util::precise_sleep(test_util::TimeUnit(1));
        }
    }

    std::atomic<int> idles_fired{0};

    std::atomic<bool> sync_obtained{false};
    std::atomic<bool> main_idle{false};
    std::atomic<bool> default_idle{false};
};


TEST_CASE("Test that Idle won't fire when an already idle pool goes idle again", "[api][dsl][Idle][Pool]") {

    NUClear::Configuration config;
    config.default_pool_concurrency = 1;
    NUClear::PowerPlant plant(config);
    test_util::add_tracing(plant);
    auto& reactor = plant.install<TestReactor>();
    plant.start();

    CHECK(reactor.idles_fired == 1);
}
