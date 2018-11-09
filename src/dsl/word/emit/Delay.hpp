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

#ifndef NUCLEAR_DSL_WORD_EMIT_DELAY_HPP
#define NUCLEAR_DSL_WORD_EMIT_DELAY_HPP

#include "../../operation/ChronoTask.hpp"
#include "Direct.hpp"

namespace NUClear {
namespace dsl {
    namespace word {
        namespace emit {

            /**
             * @brief
             *  This will emit data, after the provided delay.
             *
             * @details
             *  @code emit<Scope::DELAY>(data, delay(ticks), dataType); @endcode
             *  Emissions under this scope will wait for the provided time delay, and then emit the object utilising a
             *  local emit (that is, normal threadpool distribution).
             *
             * @param data
             *  the data to emit
             * @param delay(ticks)
             *  the time to wait before emitting this object. Use delay to specify the unit in which to measure the
             *  ticks, this will default to clock duration, but can accept any of the defined std::chrono durations
             *  (nanoseconds, microseconds, milliseconds, seconds, minutes, hours). Note that you can also define your
             *  own unit:  See http://en.cppreference.com/w/cpp/chrono/duration. Use an int to specify the number of
             *  ticks to wait.
             * @tparam DataType
             *  the datatype of the object to emit
             */
            template <typename DataType>
            struct Delay {

                static void emit(PowerPlant& powerplant,
                                 std::shared_ptr<DataType> data,
                                 NUClear::clock::duration delay) {

                    // Our chrono task is just to do a normal emit in the amount of time
                    auto msg = std::make_shared<operation::ChronoTask>(
                        [&powerplant, data](NUClear::clock::time_point&) {
                            // Do the emit
                            emit::Local<DataType>::emit(powerplant, data);

                            // We don't renew, remove us
                            return false;
                        },
                        NUClear::clock::now() + delay,
                        -1);  // Our ID is -1 as we will remove ourselves

                    // Send this straight to the chrono controller
                    emit::Direct<operation::ChronoTask>::emit(powerplant, msg);
                }

                static void emit(PowerPlant& powerplant,
                                 std::shared_ptr<DataType> data,
                                 NUClear::clock::time_point at_time) {

                    // Our chrono task is just to do a normal emit in the amount of time
                    auto msg = std::make_shared<operation::ChronoTask>(
                        [&powerplant, data](NUClear::clock::time_point&) {
                            // Do the emit
                            emit::Local<DataType>::emit(powerplant, data);

                            // We don't renew, remove us
                            return false;
                        },
                        at_time,
                        -1);  // Our ID is -1 as we will remove ourselves

                    // Send this straight to the chrono controller
                    emit::Direct<operation::ChronoTask>::emit(powerplant, msg);
                }
            };

        }  // namespace emit
    }      // namespace word
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_WORD_EMIT_DELAY_HPP
