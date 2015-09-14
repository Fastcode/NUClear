/*
 * Copyright (C) 2013 Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
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

#ifndef NUCLEAR_DSL_WORD_ALWAYS_H
#define NUCLEAR_DSL_WORD_ALWAYS_H

#include "nuclear_bits/util/get_identifier.h"

namespace NUClear {
    namespace dsl {
        namespace word {

            /**
             * @ingroup SmartTypes
             * @brief This type is used to make a task that will continually run until shutdown.
             *
             * @details
             *  This will start up when the system starts up and continue to execute continually
             *  until the whole system is told to shut down.
             */
            struct Always {
                
                template <typename DSL, typename TFunc>
                static inline threading::ReactionHandle bind(Reactor& reactor, const std::string& label, TFunc&& callback) {
                    
                    // Get our identifier string
                    std::vector<std::string> identifier = util::get_identifier<typename DSL::DSL, TFunc>(label);
                    
                    auto unbinder = [] (threading::Reaction& r) {
                        r.enabled = false;
                    };
                    
                    // Create our reaction and store it in the TypeCallbackStore
                    auto reaction = std::make_shared<threading::Reaction>(std::move(identifier), std::forward<TFunc>(callback), DSL::precondition, DSL::priority, DSL::postcondition, std::move(unbinder));
                    threading::ReactionHandle handle(reaction.get());
                    
                    // A labmda that will get a reaction task
                    auto run = [reaction] {
                        auto task = reaction->getTask();
                        (*task)();
                    };
                    
                    // This is our function that runs forever until the powerplant exits
                    auto loop = [&reactor, run] {
                        while(reactor.powerplant.running()) {
                            try {
                                run();
                            }
                            catch(util::CancelRunException ex) {
                            }
                            catch(...) {
                            }
                        }
                    };
                    
                    reactor.powerplant.addThreadTask(loop);
                    
                    // Return our handle
                    return handle;
                }
            };
        }
    }
}

#endif
