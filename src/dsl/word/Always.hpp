/*
 * Copyright (C) 2013      Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
 *               2014-2022 Trent Houliston <trent@houliston.me>
 *               2023      Trent Houliston <trent@houliston.me>, Alex Biddulph <bidskii@gmail.com>
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

#include "../../threading/ReactionTask.hpp"
#include "../../util/ThreadPoolDescriptor.hpp"

namespace NUClear {
namespace dsl {
    namespace word {

        /**
         * @brief
         *  This is used to request any continuous reactions in the system.
         *
         * @details
         *  @code on<Always> @endcode
         *  This request will ensure a single instance of the associated reaction is running at all times.
         *  That is, as one instance is completed, a new instance of the task will spawn.
         *
         *  Any reactions requested using this keyword will initialise upon system start-up and execute continually
         *  until system shut-down.
         *
         *  Note that a task spawned from this request will execute in its own unique thread rather than the default
         *  thread pool.
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
         *  the reaction interruptible with an on<Shutdown> reaction.  This will enforce a clean shutdown in the system.
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
            static inline util::ThreadPoolDescriptor pool(threading::Reaction& reaction) {
                static std::map<uint64_t, uint64_t> pool_id;
                static std::mutex mutex;

                const std::lock_guard<std::mutex> lock(mutex);
                if (pool_id.count(reaction.id) == 0) {
                    pool_id[reaction.id] = util::ThreadPoolDescriptor::get_unique_pool_id();
                }
                return util::ThreadPoolDescriptor{pool_id[reaction.id], 1};
            }

            template <typename DSL>
            static inline void bind(const std::shared_ptr<threading::Reaction>& always_reaction) {
                /**
                 * Static map mapping reaction id (from the always reaction) to a pair of reaction pointers -- one for
                 * the always reaction and one for the idle reaction that we generate in this function
                 * The main purpose of this map is to ensure that the always reaction pointer doesn't get destroyed
                 */
                static std::map<uint64_t,
                                std::pair<std::shared_ptr<threading::Reaction>, std::shared_ptr<threading::Reaction>>>
                    reaction_store = {};

                /**
                 * Generate a new reaction for an idle task
                 * The purpose of this reaction is to ensure that the always reaction is resubmitted in the event that
                 * the precondition fails (e.g. on<Always, With<X>> will fail the precondition if there are no X
                 * messages previously emitted)
                 *
                 * In the event that the precondition on the always reaction fails this idle task will run and resubmit
                 * both the always reaction and the idle reaction
                 *
                 * The idle reaction must have a lower priority than the always reaction and must also run in the same
                 * thread pool and group as the always reaction
                 */
                auto idle_reaction = std::make_shared<threading::Reaction>(
                    always_reaction->reactor,
                    std::vector<std::string>{always_reaction->identifier[0] + " - IDLE Task",
                                             always_reaction->identifier[1],
                                             always_reaction->identifier[2],
                                             always_reaction->identifier[3]},
                    [always_reaction](threading::Reaction& idle_reaction) -> util::GeneratedCallback {
                        auto callback = [&idle_reaction, always_reaction](threading::ReactionTask& /*task*/) {
                            // Get a task for the always reaction and submit it to the scheduler
                            auto always_task = always_reaction->get_task();
                            if (always_task) {
                                always_reaction->reactor.powerplant.submit(std::move(always_task));
                            }

                            // Get a task for the idle reaction and submit it to the scheduler
                            auto idle_task = idle_reaction.get_task();
                            if (idle_task) {
                                // Set the thread pool on the task
                                idle_task->thread_pool_descriptor = DSL::pool(*always_reaction);

                                // Make sure that idle reaction always has lower priority than the always reaction
                                idle_task->priority = DSL::priority(*always_reaction) - 1;

                                // Submit the task to be run
                                idle_reaction.reactor.powerplant.submit(std::move(idle_task));
                            }
                        };

                        // Make sure that idle reaction always has lower priority than the always reaction
                        return {DSL::priority(*always_reaction) - 1,
                                DSL::group(*always_reaction),
                                DSL::pool(*always_reaction),
                                callback};
                    });

                // Don't emit stats for the idle reaction
                idle_reaction->emit_stats = false;

                // Keep this reaction handy so it doesn't go out of scope
                reaction_store[always_reaction->id] = {always_reaction, idle_reaction};

                // Create an unbinder for the always reaction
                always_reaction->unbinders.push_back([](threading::Reaction& r) {
                    r.enabled = false;
                    reaction_store.erase(r.id);
                    // TODO(Alex/Trent) Clean up thread pool too
                });

                // Get a task for the always reaction and submit it to the scheduler
                auto always_task = always_reaction->get_task();
                if (always_task) {
                    always_reaction->reactor.powerplant.submit(std::move(always_task));
                }

                // Get a task for the idle reaction and submit it to the scheduler
                auto idle_task = idle_reaction->get_task();
                if (idle_task) {
                    idle_reaction->reactor.powerplant.submit(std::move(idle_task));
                }
            }

            template <typename DSL>
            static inline void postcondition(threading::ReactionTask& task) {
                // Get a task for the always reaction and submit it to the scheduler
                auto new_task = task.parent.get_task();
                if (new_task) {
                    task.parent.reactor.powerplant.submit(std::move(new_task));
                }
            }
        };

    }  // namespace word
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_WORD_ALWAYS_HPP
