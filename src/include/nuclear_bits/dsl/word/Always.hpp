/*
 * Copyright (C) 2013-2016 Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
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

#include "nuclear_bits/util/get_identifier.hpp"

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
         *  Note that a task spawned from this request will execute in its own unique thread rather than the threadpool.
         *  However, if the task is rescheduled (such as with Sync), it will then be moved into the threadpool.
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
         *  used when there is no other way to scheduled the reaction.  If a developer is tempted to use this keyword,
         *  it is advised to review other options, such as on<IO> before resorting to this feature.
         *
         * @par Implements
         *  Bind
         */
        struct Always {

            template <typename DSL, typename Function>
            static inline threading::ReactionHandle bind(Reactor& reactor,
                                                         const std::string& label,
                                                         Function&& callback) {

                // Get our identifier string
                std::vector<std::string> identifier =
                    util::get_identifier<typename DSL::DSL, Function>(label, reactor.reactor_name);

                auto unbinder = [](threading::Reaction& r) { r.enabled = false; };

                // Create our reaction and store it in the TypeCallbackStore
                auto reaction = std::make_shared<threading::Reaction>(
                    reactor, std::move(identifier), std::forward<Function>(callback), std::move(unbinder));
                threading::ReactionHandle handle(reaction);

                // A lambda that will get a reaction task
                auto run = [reaction] {
                    // Get a task
                    auto task = reaction->get_task();

                    // If we got a real task back
                    if (task) {
                        task = task->run(std::move(task));
                    }
                };

                // This is our function that runs forever until the powerplant exits
                auto loop = [&reactor, run] {
                    while (reactor.powerplant.running()) {
                        try {
                            run();
                        }
                        catch (...) {
                        }
                    }
                };

                reactor.powerplant.add_thread_task(loop);

                // Return our handle
                return handle;
            }
        };

    }  // namespace word
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_WORD_ALWAYS_HPP
