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

#include "nuclear_bits/dsl/operation/Unbind.hpp"
#include "nuclear_bits/dsl/store/DataStore.hpp"
#include "nuclear_bits/dsl/word/emit/Direct.hpp"
#include "nuclear_bits/message/ServiceWatchdog.hpp"
#include "nuclear_bits/util/generate_reaction.hpp"

namespace NUClear {
namespace dsl {
    namespace word {

        template <typename TWatchdog, int ticks, class period>

        /**
         * @brief
         *  This can be used to monitor tasks/s; if the monitored task/s have not occurred within a desired timeframe,
         *  the watchdog can be serviced to trigger a specified reaction.
         *
         * @details
         *  @code on<Watchdog<TWatchdog, ticks, period>>() @endcode
         *  This is a useful tool for anything in the system which might stall, and needs to be kick-started.
         *
         *  The watchdog can monitor a single task, or group of tasks, over a period of time. If no activity is
         *  detected after the specified timeframe, the watchdog will be serviced.  When the watchdog is serviced, the
         *  timer re-sets.
         *
         * @par Single Reaction
         *  @code on<Watchdog<SampleReaction, 10, std::chrono::milliseconds>>() @endcode
         *  In the example above, a SampleReaction will be monitored.  If the reactions does not occur within 10
         *  milliseconds, the watchdog will be serviced.
         *
         * @par Group of Reactions
         *  @code on<Watchdog<SampleReactor, 10, std::chrono::milliseconds>>() @endcode
         *  In the example above, all reactions from the SampleReactor will be monitored.  If a task associated with the
         *  SampleReactor has not occurred for 10 milliseconds,  the watchdog will be serviced.
         *
         * @par Service the Watcdog
         *  @code  emit(std::make_unique<NUClear::message::ServiceWatchdog<SampleReaction>>()) @endcode
         *  or
         *  @code  emit(std::make_unique<NUClear::message::ServiceWatchdog<SampleReactor>>()) @endcode
         *  The watchdog will need to be serviced by a watchdog service emission. The emission must use the same
         *  template type as the watchdog.  Each time this emission occurs, the watchdog timer will be reset.
         *
         * @attention
         *  The period which is used to measure the ticks must be greater than or equal to clock::duration or the
         *  program will not compile.
         *
         * @par Implements
         *  Bind, Get
         *
         * @tparam TWatchdog
         *  the type/group of tasks the watchdog will track.   This needs to be a declared type within the system (be it
         *  a reactor, reaction, or other type).
         * @tparam ticks
         *  the number of ticks of a particular type to wait
         * @tparam period
         *  a type of duration (e.g. std::chrono::seconds) to measure the ticks in.  This will default to clock
         *  duration, but can accept any of the defined std::chrono durations (nanoseconds, microseconds, milliseconds,
         *  seconds, minutes, hours).  Note that you can also define your own unit:  See
         *  http://en.cppreference.com/w/cpp/chrono/duration
         */
        struct Watchdog {

            template <typename DSL, typename Function>
            static inline threading::ReactionHandle bind(Reactor& reactor,
                                                         const std::string& label,
                                                         Function&& callback) {

                // If this is the first time we have used this watchdog service it
                if (!store::DataStore<message::ServiceWatchdog<TWatchdog>>::get()) {
                    reactor.powerplant.emit(std::make_unique<message::ServiceWatchdog<TWatchdog>>());
                }

                // Build our reaction
                auto reaction =
                    std::shared_ptr<threading::Reaction>(util::generate_reaction<DSL, operation::ChronoTask>(
                        reactor, label, std::forward<Function>(callback)));
                threading::ReactionHandle handle(reaction);

                // Send our configuration out
                reactor.powerplant.emit<emit::Direct>(std::make_unique<operation::ChronoTask>(
                    [&reactor, reaction](NUClear::clock::time_point& time) {

                        // Get the latest time the watchdog was serviced
                        auto service_time = store::DataStore<message::ServiceWatchdog<TWatchdog>>::get()->time;

                        // Check if our watchdog has timed out
                        if (NUClear::clock::now() > (service_time + period(ticks))) {
                            try {
                                // Submit the reaction to the thread pool
                                auto task = reaction->get_task();
                                if (task) {
                                    reactor.powerplant.submit(std::move(task));
                                }
                            }
                            catch (...) {
                            }

                            // Now automatically service the watchdog
                            time = NUClear::clock::now() + period(ticks);
                        }
                        // Change our wait time to our new watchdog time
                        else {
                            time = service_time + period(ticks);
                        }

                        // We renew!
                        return true;

                    },
                    NUClear::clock::now() + period(ticks),
                    reaction->id));

                // Return our handle
                return handle;
            }
        };

    }  // namespace word
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_WORD_WATCHDOG_HPP
