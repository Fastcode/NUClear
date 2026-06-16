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
#include "threading/scheduler/Scheduler.hpp"

#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <set>
#include <stdexcept>

#include "threading/ReactionTask.hpp"
#include "util/GroupDescriptor.hpp"
#include "util/Inline.hpp"
#include "util/ThreadPoolDescriptor.hpp"

namespace NUClear {
namespace threading {
    namespace scheduler {

        namespace {
            std::unique_ptr<ReactionTask> make_inline_group_task(std::shared_ptr<const util::GroupDescriptor> group_desc,
                                                                 std::atomic<int>& ran) {
                auto task = std::make_unique<ReactionTask>(
                    nullptr,
                    false,
                    [](const ReactionTask& /*task*/) { return 0; },
                    [](const ReactionTask& /*task*/) { return util::Inline::ALWAYS; },
                    [](const ReactionTask& /*task*/) {
                        return std::make_shared<util::ThreadPoolDescriptor>("InlinePool", 1, false);
                    },
                    [group_desc](const ReactionTask& /*task*/) {
                        return std::set<std::shared_ptr<const util::GroupDescriptor>>{group_desc};
                    });
                task->callback = [&ran](ReactionTask& /*task*/) { ran.fetch_add(1, std::memory_order_acq_rel); };
                return task;
            }
        }  // namespace

        SCENARIO("Creating a pool after shutdown is rejected", "[threading][scheduler][Scheduler]") {
            GIVEN("A scheduler that has begun shutting down") {
                Scheduler scheduler(1);
                scheduler.stop();

                WHEN("A never-before-seen pool descriptor is requested") {
                    auto desc = std::make_shared<util::ThreadPoolDescriptor>("LatePool", 1, false);

                    THEN("The scheduler throws rather than creating a new pool") {
                        REQUIRE_THROWS_AS(scheduler.add_idle_task(nullptr, desc), std::invalid_argument);
                    }
                }
            }
        }

        SCENARIO("A single-group inline task runs synchronously when a token is available",
                 "[threading][scheduler][Scheduler]") {
            GIVEN("A scheduler and a group with a free token") {
                Scheduler scheduler(1);
                auto group_desc = std::make_shared<util::GroupDescriptor>("InlineGroup", 1);
                std::atomic<int> ran{0};

                WHEN("An inline task for that sole group is submitted") {
                    scheduler.submit(make_inline_group_task(group_desc, ran));

                    THEN("The task callback runs on the submitting thread without queueing") {
                        CHECK(ran.load(std::memory_order_acquire) == 1);
                    }
                }
            }
        }

    }  // namespace scheduler
}  // namespace threading
}  // namespace NUClear
