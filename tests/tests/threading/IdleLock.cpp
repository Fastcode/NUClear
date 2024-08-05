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
#include <catch2/generators/catch_generators.hpp>

namespace NUClear {
namespace threading {
    namespace scheduler {

        char lock_state_to_char(const std::unique_ptr<IdleLock>& lock) {
            switch (lock->lock() << 2 | lock->local_idle() << 1 | lock->global_idle()) {
                case 0b000: return 'U';  // Unlocked
                case 0b101: return 'G';  // Global only
                case 0b110: return 'L';  // Local only
                case 0b111: return 'B';  // Both locked
                case 0b001: return 'g';  // Error global without being locked
                case 0b010: return 'l';  // Error local without being locked
                case 0b011: return 'b';  // Error both without being locked
                case 0b100: return 'u';  // Error locked without a reason
                default: return '?';
            }
        }

        void check_locks(const std::string& name,
                         const std::vector<std::unique_ptr<IdleLock>>& locks,
                         const std::string& expected) {
            INFO("Checking " << name << " Locks");

            // Make the expected string from the locks
            std::string actual;
            for (const auto& lock : locks) {
                actual += lock_state_to_char(lock);
            }

            // Check it matches
            CHECK(actual == expected);
        }

        struct CheckSemaphoreState {
            unsigned int current;
            unsigned int expected;
            bool expected_locked;
            CheckSemaphoreState(unsigned int current, unsigned int expected, bool expected_locked)
                : current(current), expected(expected), expected_locked(expected_locked) {}
        };
        void check_semaphore_state(const CheckSemaphoreState& v) {
            // The lock value is stored in the most significant bit of the integer
            unsigned int mask  = (1 << (sizeof(v.current) * 8 - 1));
            bool locked        = (v.current & mask) == mask;
            unsigned int count = v.current & ~mask;
            CHECK(count == v.expected);
            CHECK(locked == v.expected_locked);
        }

        void check_var_states(const std::string& msg,
                              const CheckSemaphoreState& local,
                              const CheckSemaphoreState& global) {

            {
                INFO("Checking " << msg << " Local States");
                check_semaphore_state(local);
            }
            {
                INFO("Checking " << msg << " Global States");
                check_semaphore_state(global);
            }
        }

        TEST_CASE("IdleLock correctly tracks the state of the local and global pools",
                  "[threading][scheduler][IdleLock]") {

            constexpr unsigned int n_locks = 3;

            unsigned int a = n_locks;
            unsigned int b = n_locks;
            std::atomic<unsigned int> g(n_locks * 2);

            std::vector<std::unique_ptr<IdleLock>> a_locks;
            std::vector<std::unique_ptr<IdleLock>> b_locks;

            {
                INFO("Locking enough for all except for the last lock");

                // Perform all except the last lock checking that the state is correct at each step
                for (unsigned int i = 0; i + 1 < n_locks; i++) {
                    INFO("Locking Step " << i);
                    unsigned int e = n_locks - i - 1;  // Expect to be one less after locking

                    a_locks.push_back(std::make_unique<IdleLock>(a, g));
                    check_var_states("A", {a, e, false}, {g.load(), e * 2 + 1, false});
                    CHECK(a_locks.back()->lock() == false);

                    b_locks.push_back(std::make_unique<IdleLock>(b, g));
                    check_var_states("B", {b, e, false}, {g.load(), e * 2, false});
                    CHECK(b_locks.back()->lock() == false);
                }

                check_locks("A", a_locks, "UU");
                check_locks("B", b_locks, "UU");
            }

            {
                INFO("Locking the final locks");

                a_locks.push_back(std::make_unique<IdleLock>(a, g));
                check_var_states("A", {a, 0, true}, {g.load(), 1, false});
                check_locks("A", a_locks, "UUL");

                b_locks.push_back(std::make_unique<IdleLock>(b, g));
                check_var_states("B", {b, 0, true}, {g.load(), 0, true});
                check_locks("B", b_locks, "UUB");
            }

            {
                INFO("Clearing the first (unlocked) lock from each list");
                a_locks.erase(a_locks.begin());
                check_var_states("A", {a, 1, true}, {g.load(), 1, true});
                check_locks("A", a_locks, "UL");

                b_locks.erase(b_locks.begin());
                check_var_states("B", {b, 1, true}, {g.load(), 2, true});
                check_locks("B", b_locks, "UB");
            }

            {
                INFO("Relocking the first lock in each list (should not gain the lock)");

                a_locks.insert(a_locks.begin(), std::make_unique<IdleLock>(a, g));
                check_var_states("A", {a, 0, true}, {g.load(), 1, true});
                check_locks("A", a_locks, "UUL");

                b_locks.insert(b_locks.begin(), std::make_unique<IdleLock>(b, g));
                check_var_states("B", {b, 0, true}, {g.load(), 0, true});
                check_locks("B", b_locks, "UUB");
            }

            {
                INFO("Unlocking empty a lock and b global lock");

                a_locks.erase(a_locks.begin());
                check_var_states("A", {a, 1, true}, {g.load(), 1, true});
                check_locks("A", a_locks, "UL");

                b_locks.pop_back();
                check_var_states("B", {b, 1, false}, {g.load(), 2, false});
                check_locks("B", b_locks, "UU");
            }
            {
                INFO("Locking B as local, and a new A as global only");

                b_locks.emplace_back(std::make_unique<IdleLock>(b, g));
                check_var_states("B", {b, 0, true}, {g.load(), 1, false});
                check_locks("B", b_locks, "UUL");

                a_locks.emplace_back(std::make_unique<IdleLock>(a, g));
                check_var_states("A", {a, 0, true}, {g.load(), 0, true});
                check_locks("A", a_locks, "ULG");
            }
            {
                INFO("Unlocking the local only A lock and relocking it");
                a_locks.erase(std::next(a_locks.begin(), 1));
                check_var_states("A", {a, 1, false}, {g.load(), 1, true});
                check_locks("A", a_locks, "UG");

                a_locks.emplace_back(std::make_unique<IdleLock>(a, g));
                check_var_states("A", {a, 0, true}, {g.load(), 0, true});
                check_locks("A", a_locks, "UGL");
            }
            {
                INFO("Unlocking all remaining locks");
                a_locks.clear();
                check_var_states("A", {a, n_locks, false}, {g.load(), n_locks, false});

                b_locks.clear();
                check_var_states("B", {b, n_locks, false}, {g.load(), n_locks * 2, false});
            }
        }

    }  // namespace scheduler
}  // namespace threading
}  // namespace NUClear
