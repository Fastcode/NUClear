/*
 * Copyright (C) 2013      Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
 *               2014-2017 Trent Houliston <trent@houliston.me>
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
#include "../operation/ChronoTask.hpp"
#include "../operation/Unbind.hpp"
#include "emit/Direct.hpp"

namespace NUClear {
namespace dsl {
    namespace word {

        /**
         * @brief
         *  This type is used within an every in order to measure a frequency rather then a period.
         */
        template <typename period>
        struct Per;

        template <typename Unit, std::intmax_t num, std::intmax_t den>
        struct Per<std::chrono::duration<Unit, std::ratio<num, den>>> : public clock::duration {
            Per(int ticks)
                : clock::duration(std::lround((double(num) / double(ticks* den))
                                              * (double(clock::period::den) / double(clock::period::num)))) {}
        };

        /**
         * @brief
         *  This is used to request any periodic reactions in the system.
         *
         * @details
         *  @code on<Every<ticks, period>>() @endcode
         *  This request will enact the execution of a task at a periodic rate. To set the timing, simply specify the
         *  desired period with the request.  For example, to run a task every two seconds, the following request would
         *  be used:
         *
         *  @code on<Every<2, std::chrono::seconds>() @endcode
         *
         *  Note that the period argument can also be wrapped in a Per<> template so that the inverse relation
         *  can be invoked.  For instance, to execute a callback to initialise two tasks every second, then the
         *  request would be used:
         *  @code on<Every<2, Per<std::chrono::seconds>>() @endcode
         *
         * @attention
         *  The period which is used to measure the ticks must be greater than or equal to clock::duration or the
         *  program will not compile.
         *
         * @par Implements
         *  Bind
         *
         * @tparam ticks
         *  the number of ticks of a particular type to wait
         * @tparam period
         *  a type of duration (e.g. std::chrono::seconds) to measure the ticks in.  This will default to clock
         *  duration, but can accept any of the defined std::chrono durations (nanoseconds, microseconds, milliseconds,
         *  seconds, minutes, hours).  Note that you can also define your own unit:  See
         *  http://en.cppreference.com/w/cpp/chrono/duration
         */
        template <int ticks = 0, class period = NUClear::clock::duration>
        struct Every;

        template <>
        struct Every<0, NUClear::clock::duration> {

            template <typename DSL>
            static inline void bind(const std::shared_ptr<threading::Reaction>& reaction,
                                    NUClear::clock::duration jump) {

                reaction->unbinders.push_back([](const threading::Reaction& r) {
                    r.reactor.emit<emit::Direct>(std::make_unique<operation::Unbind<operation::ChronoTask>>(r.id));
                });

                // Send our configuration out
                reaction->reactor.emit<emit::Direct>(std::make_unique<operation::ChronoTask>(
                    [reaction, jump](NUClear::clock::time_point& time) {
                        try {
                            // submit the reaction to the thread pool
                            auto task = reaction->get_task();
                            if (task) {
                                reaction->reactor.powerplant.submit(std::move(task));
                            }
                        }
                        // If there is an exception while generating a reaction print it here, this shouldn't happen
                        catch (const std::exception& ex) {
                            reaction->reactor.log<NUClear::ERROR>("There was an exception while generating a reaction",
                                                                  ex.what());
                        }
                        catch (...) {
                            reaction->reactor.log<NUClear::ERROR>(
                                "There was an unknown exception while generating a reaction");
                        }

                        time += jump;

                        return true;
                    },
                    NUClear::clock::now() + jump,
                    reaction->id));
            }
        };

        template <int ticks, class period>
        struct Every {

            template <typename DSL>
            static inline void bind(const std::shared_ptr<threading::Reaction>& reaction) {
                Every<>::bind<DSL>(reaction, period(ticks));
            }
        };

    }  // namespace word
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_WORD_EVERY_HPP
