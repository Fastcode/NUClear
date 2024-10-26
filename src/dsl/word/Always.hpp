/*
 * MIT License
 *
 * Copyright (c) 2014 NUClear Contributors
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

#ifndef NUCLEAR_DSL_WORD_ALWAYS_HPP
#define NUCLEAR_DSL_WORD_ALWAYS_HPP

#include <map>
#include <mutex>
#include <utility>

#include "../../id.hpp"
#include "../../threading/ReactionIdentifiers.hpp"
#include "../../threading/ReactionTask.hpp"
#include "../../util/Inline.hpp"
#include "../../util/ThreadPoolDescriptor.hpp"

namespace NUClear {
namespace dsl {
    namespace word {

        /**
         * This is used to request any continuous reactions in the system.
         *
         * @code on<Always> @endcode
         * This request will ensure a single instance of the associated reaction is running at all times.
         * That is, as one instance is completed, a new instance of the task will spawn.
         *
         * Any reactions requested using this keyword will initialise upon system start-up and execute continually
         * until system shut-down.
         *
         * @note
         *  A task spawned from this request will execute in its own unique thread rather than the default thread pool.
         *
         * @par Infinite Loops
         *  This word should be used in place of any reactions which would contain an infinite loop. That is,
         *  <b>it is not recommended to use a while(true) loop (or equivalent) in a reaction</b>.
         *  Using this word allows the task to (cleanly) finish and restart itself, allowing the task to
         *  terminate properly when the system is shutdown.  Note that tasks which do not terminate correctly
         *  at system shutdown will cause the system to hang.
         *
         * @par Ensure Clean Shutdown
         *  If the reaction associated with this task is performing a blocking operation, developers should make the
         *  the reaction interruptable with an on<Shutdown> reaction.  This will enforce a clean shutdown in the system.
         *
         * @attention
         *  Where possible, developers should <b>avoid using this keyword</b>.  It has been provided, but should only be
         *  used when there is no other way to schedule the reaction.  If a developer is tempted to use this keyword,
         *  it is advised to review other options, such as on<IO> before resorting to this feature.
         *
         * @par Implements
         *  Pool
         *  Bind
         */
        struct Always {

            template <typename DSL>
            static std::shared_ptr<const util::ThreadPoolDescriptor> pool(const threading::ReactionTask& task) {
                static std::map<NUClear::id_t, std::shared_ptr<util::ThreadPoolDescriptor>> pools;
                static std::mutex mutex;
                const auto& reaction = *task.parent;

                const std::lock_guard<std::mutex> lock(mutex);
                if (pools.count(reaction.id) == 0) {

                    const std::string pool_name = !reaction.identifiers->name.empty()
                                                      ? std::string(reaction.identifiers->name)
                                                      : std::string("Always[") + std::to_string(reaction.id) + "]";

                    pools[reaction.id] = std::make_shared<util::ThreadPoolDescriptor>(pool_name, 1, false);
                }
                return pools.at(reaction.id);
            }

            template <typename DSL>
            static util::Inline run_inline(const threading::ReactionTask& /*task*/) {
                return util::Inline::NEVER;
            }

            template <typename DSL>
            static void bind(const std::shared_ptr<threading::Reaction>& reaction) {

                // Create an unbinder for the always reaction
                reaction->unbinders.push_back([](threading::Reaction& r) {
                    r.enabled = false;
                    // TODO(Alex/Trent) Clean up thread pool too
                });

                // Submit the always and idle task to the scheduler
                PowerPlant::powerplant->submit(reaction->get_task());
                PowerPlant::powerplant->submit(make_idle_task<DSL>(reaction));
            }

            template <typename DSL>
            static void post_run(threading::ReactionTask& task) {
                // Get a task for the always reaction and submit it to the scheduler
                PowerPlant::powerplant->submit(task.parent->get_task());
            }

        private:
            /**
             * Generate an idle task for Always which will be used to resubmit the Always task if it fails
             *
             * @tparam DSL      the DSL that the Always task is using
             * @param reaction  the reaction that the Always task is associated with
             *
             * @return a unique pointer to the idle task which will resubmit the Always task and itself
             */
            template <typename DSL>
            static std::unique_ptr<threading::ReactionTask> make_idle_task(
                const std::shared_ptr<threading::Reaction>& reaction) {

                auto idle_task = std::make_unique<threading::ReactionTask>(
                    reaction,
                    false,
                    [](threading::ReactionTask& task) { return DSL::priority(task) - 1; },
                    DSL::run_inline,
                    DSL::pool,
                    DSL::group);

                idle_task->callback = [](threading::ReactionTask& task) {
                    // Submit the always and idle tasks to the scheduler
                    PowerPlant::powerplant->submit(task.parent->get_task());
                    PowerPlant::powerplant->submit(make_idle_task<DSL>(task.parent));
                };

                return idle_task;
            }
        };

    }  // namespace word
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_WORD_ALWAYS_HPP
