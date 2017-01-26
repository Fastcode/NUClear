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

#ifndef NUCLEAR_DSL_WORD_EMIT_INITIALISE_HPP
#define NUCLEAR_DSL_WORD_EMIT_INITIALISE_HPP

#include "Direct.hpp"

namespace NUClear {
namespace dsl {
    namespace word {
        namespace emit {

            /**
             * @brief Emit an object as the system starts up.
             *
             * @details Initialise emits can only be done before in the main phase of execution. Tasks that are
             *          emitted using this will be executed just before the system starts into the main phase.
             *          This can be useful for cases when a Reactor wants to emit to all other reactors but they may
             *          not yet be installed. Using this allows the message to wait until all Reactors are installed
             *          and then emit.
             *
             * @param data the data to emit
             *
             * @tparam DataType the type of the data to be emitted
             */
            template <typename DataType>
            struct Initialise {

                static void emit(PowerPlant& powerplant, std::shared_ptr<DataType> data) {

                    auto task = [&powerplant, data] { emit::Direct<DataType>::emit(powerplant, data); };

                    powerplant.onStartup(task);
                }
            };

        }  // namespace emit
    }      // namespace word
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_WORD_EMIT_INITIALISE_HPP
