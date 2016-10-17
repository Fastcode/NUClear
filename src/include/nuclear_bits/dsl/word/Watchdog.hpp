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

#ifndef NUCLEAR_DSL_WORD_WATCHDOG_HPP
#define NUCLEAR_DSL_WORD_WATCHDOG_HPP

#include "nuclear_bits/message/ServiceWatchdog.hpp"
#include "nuclear_bits/dsl/store/DataStore.hpp"
#include "nuclear_bits/dsl/operation/Unbind.hpp"
#include "nuclear_bits/dsl/word/emit/Direct.hpp"
#include "nuclear_bits/util/generate_reaction.hpp"

namespace NUClear {
    namespace dsl {
        namespace word {
            
            template <typename TWatchdog, int ticks, class period>
            struct Watchdog {

                template <typename DSL, typename TFunc>
                static inline threading::ReactionHandle bind(Reactor& reactor, const std::string& label, TFunc&& callback) {
                    
                    // If this is the first time we have used this watchdog service it
                    if (!store::DataStore<message::ServiceWatchdog<TWatchdog>>::get()) {
                        reactor.powerplant.emit(std::make_unique<message::ServiceWatchdog<TWatchdog>>());
                    }

                    // Build our reaction
                    auto reaction = std::shared_ptr<threading::Reaction>(util::generate_reaction<DSL, operation::ChronoTask>(reactor, label, std::forward<TFunc>(callback)));
                    threading::ReactionHandle handle(reaction);
                    
                    // Send our configuration out
                    reactor.powerplant.emit<emit::Direct>(std::make_unique<operation::ChronoTask>([&reactor, reaction] (NUClear::clock::time_point& time) {
                        
                        // Get the latest time the watchdog was serviced
                        auto service_time = store::DataStore<message::ServiceWatchdog<TWatchdog>>::get()->time;
                        
                        // Check if our watchdog has timed out
                        if (time > (service_time + period(ticks))) {
                            try {
                                // Submit the reaction to the thread pool
                                auto task = reaction->getTask();
                                if(task) {
                                    reactor.powerplant.submit(std::move(task));
                                }
                            }
                            catch(...) {
                            }
                            
                            time = NUClear::clock::now() + period(ticks);
                        }
                        // Change our wait time to our new watchdog time
                        else {
                            time = service_time + period(ticks);
                        }
                        
                        // We renew!
                        return true;
                        
                    }, NUClear::clock::now() + period(ticks), reaction->reactionId));
                    
                    // Return our handle
                    return handle;
                }
            };

        }  // namespace word
    }  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_WORD_WATCHDOG_HPP
