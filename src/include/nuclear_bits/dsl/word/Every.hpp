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

#ifndef NUCLEAR_DSL_WORD_EVERY_HPP
#define NUCLEAR_DSL_WORD_EVERY_HPP

#include <cmath>
#include "nuclear_bits/dsl/operation/Unbind.hpp"
#include "nuclear_bits/dsl/operation/ChronoTask.hpp"
#include "nuclear_bits/dsl/word/emit/Direct.hpp"
#include "nuclear_bits/util/generate_reaction.hpp"

namespace NUClear {
    namespace dsl {
        namespace word {

            /**
             * @brief This type is used within an every in order to measure a frequency rather then a period.
             *
             * @author Trent Houliston
             */
            template <typename period>
            struct Per;

            template <typename Unit, std::intmax_t num, std::intmax_t den>
            struct Per<std::chrono::duration<Unit, std::ratio<num, den>>> : public clock::duration {
                Per(int ticks) : clock::duration(std::lround((double(num) / double(ticks * den)) * (double(clock::period::den) / double(clock::period::num)))) {}
            };

            /**
             * @ingroup SmartTypes
             * @brief A special flag type which specifies that this reaction should trigger at the given rate
             *
             * @details
             *  This type is used in a dsl statement to specify that the given reaction should trigger at the rate
             *  set by this type. For instance, if the type was specified as shown in the following example
             *  @code on<Every<2, std::chrono::seconds> @endcode
             *  then the callback would execute every 2 seconds. This type simply needs to exist in the trigger for the
             *  correct timing to be called.
             *
             * @attention Note that the period which is used to measure the ticks in must be greater than or equal to
             *  clock::duration or the program will not compile
             *
             * @tparam ticks the number of ticks of a paticular type to wait
             * @tparam period a type of duration (e.g. std::chrono::seconds) to measure the ticks in, defaults to clock duration.
             *                  This paramter may also be wrapped in a Per<> template in order to write a frequency rather then a period.
             */
            template <int ticks = 0, class period = NUClear::clock::duration>
            struct Every;

            template <>
            struct Every<0, NUClear::clock::duration> {

                template <typename DSL, typename TFunc>
                static inline threading::ReactionHandle bind(Reactor& reactor, const std::string& label, TFunc&& callback, NUClear::clock::duration jump) {

                    auto reaction = std::shared_ptr<threading::Reaction>(util::generate_reaction<DSL, operation::ChronoTask>(reactor, label, std::forward<TFunc>(callback)));

                    threading::ReactionHandle handle(reaction);
                    
                    // Send our configuration out
                    reactor.powerplant.emit<emit::Direct>(std::make_unique<operation::ChronoTask>([&reactor, jump, reaction] (NUClear::clock::time_point& time) {
                        
                        try {
                            // submit the reaction to the thread pool
                            auto task = reaction->getTask();
                            if(task) {
                                reactor.powerplant.submit(std::move(task));
                            }
                        }
                        catch(...) {
                        }

                        time += jump;
                        
                        return true;
                    }, NUClear::clock::now() + jump, reaction->reactionId));

                    // Return our handle
                    return handle;
                }
            };

            template <int ticks, class period>
            struct Every {

                template <typename DSL, typename TFunc>
                static inline threading::ReactionHandle bind(Reactor& reactor, const std::string& label, TFunc&& callback) {

                    // Work out our Reaction timing
                    clock::duration jump = period(ticks);
                    
                    auto reaction = std::shared_ptr<threading::Reaction>(util::generate_reaction<DSL, operation::ChronoTask>(reactor, label, std::forward<TFunc>(callback)));
                    
                    threading::ReactionHandle handle(reaction);
                    
                    // Send our configuration out
                    reactor.powerplant.emit<emit::Direct>(std::make_unique<operation::ChronoTask>([&reactor, jump, reaction] (NUClear::clock::time_point& time) {
                        
                        try {
                            // submit the reaction to the thread pool
                            auto task = reaction->getTask();
                            if(task) {
                                reactor.powerplant.submit(std::move(task));
                            }
                        }
                        catch(...) {
                        }
                        
                        time += jump;
                        
                        return true;
                    }, NUClear::clock::now() + jump, reaction->reactionId));
                    
                    // Return our handle
                    return handle;
                }
            };

        }  // namespace word
    }  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_WORD_EVERY_HPP
