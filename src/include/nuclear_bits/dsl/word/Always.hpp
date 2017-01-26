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
        *   This is used to request any continuous reactions in the system.
         *
         * @details
         *  @code on<Always> @endcode
         *  Any reactions requested using this keyword will start when the system starts up and execute continually
         *  until the whole system is told to shut down. Note that a task spawned from this request will execute in its
         *  own unique thread rather than the thread pool. However, if the task is rescheduled (such as with Sync), it
         *  will then be moved into the thread pool.
         *
         *@attention
         *  Use of this keyworkd is preferred to keeing the system in a loop. It is not recommended to use a while(true)
         *  or similar loop in a reaction. Instead, use this keyword to let the task finish and restart itself. This
         *  allows the reaction to terminate properly when the system is shutdown. If a reaction is not terminated
         *  correctly during shut down, the system will hang. If the requested reaction is performing a blocking
         *  operation, try to make it interruptable with an on<Shutdown> reaction so that the program can be cleanly
         *  terminated.
         *
         * @TRENT????
         *  Can we discuss more at the next meeting?
         *
         * @par (Implements)
         *  Bind
         *
         */
        struct Always {

            template <typename DSL, typename Function>
            static inline threading::ReactionHandle bind(Reactor& reactor,
                                                         const std::string& label,
                                                         Function&& callback) {

                // Get our identifier string
                std::vector<std::string> identifier =
                    util::get_identifier<typename DSL::DSL, Function>(label, reactor.reactorName);

                auto unbinder = [](threading::Reaction& r) { r.enabled = false; };

                // Create our reaction and store it in the TypeCallbackStore
                auto reaction = std::make_shared<threading::Reaction>(
                    reactor, std::move(identifier), std::forward<Function>(callback), std::move(unbinder));
                threading::ReactionHandle handle(reaction);

                // A lambda that will get a reaction task
                auto run = [reaction] {
                    // Get a task
                    auto task = reaction->getTask();

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

                reactor.powerplant.addThreadTask(loop);

                // Return our handle
                return handle;
            }
        };

    }  // namespace word
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_WORD_ALWAYS_HPP
