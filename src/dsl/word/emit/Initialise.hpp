/*
 * MIT License
 *
 * Copyright (c) 2016 NUClear Contributors
 *
 * This file is part of the NUClear codebase.
 * See https://github.com/Fastcode/NUClear for further info.
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

#include "Delay.hpp"

namespace NUClear {
namespace dsl {
    namespace word {
        namespace emit {

            /**
             * @brief
             *  This scope emits data as the system starts up.
             *
             * @details
             *  @code emit<Scope::INITIALISE>(data, dataType); @endcode
             *  This should be used to emit any data required during system start up (that is, as the reactor is being
             *  installed into the powerPlant). When running emissions under this scope, the message will wait until
             *  all Reactors are installed into the powerPlant before triggering any callbacks based on the emission.
             *
             * @attention
             *  Tasks triggered by data emitted under this scope will only execute while the system is in the
             *  initialisation phase. These tasks are the final activity which occur before the system shifts into the
             *  execution phase.  Any emissions under this scope while the system is in the execution phase will be
             *  ignored.
             *
             * @param data
             *  The data to emit
             *
             * @tparam DataType
             *  The type of the data to be emitted
             */
            template <typename DataType>
            struct Initialise {

                static void emit(PowerPlant& powerplant, std::shared_ptr<DataType> data) {

                    // Submit a task to the power plant to emit this object
                    powerplant.submit(threading::ReactionTask::new_task_id(),
                                      1000,
                                      util::GroupDescriptor{},
                                      util::ThreadPoolDescriptor{},
                                      false,
                                      [&powerplant, data] { powerplant.emit_shared<dsl::word::emit::Local>(data); });
                }
            };

        }  // namespace emit
    }      // namespace word
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_WORD_EMIT_INITIALISE_HPP
