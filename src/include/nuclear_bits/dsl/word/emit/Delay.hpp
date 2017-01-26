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

#ifndef NUCLEAR_DSL_WORD_EMIT_DELAY_HPP
#define NUCLEAR_DSL_WORD_EMIT_DELAY_HPP

#include "Direct.hpp"
#include "nuclear_bits/dsl/operation/ChronoTask.hpp"

namespace NUClear {
namespace dsl {
    namespace word {
        namespace emit {

            /**
             * @brief Emits the passed object after the provided delay.
             *
             * @details Delay emits will wait the provided time delay and then emit the object utilising a normal
             *          local emit.
             *
             * @param data  the data to emit
             * @param delay the amount of time to wait before emitting this object
             *
             * @tparam DataType the datatype of the object to emit
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
            };

        }  // namespace emit
    }      // namespace word
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_WORD_EMIT_DELAY_HPP
