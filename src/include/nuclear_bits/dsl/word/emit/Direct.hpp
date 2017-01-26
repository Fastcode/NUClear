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

#ifndef NUCLEAR_DSL_WORD_EMIT_DIRECT_HPP
#define NUCLEAR_DSL_WORD_EMIT_DIRECT_HPP

#include "nuclear_bits/PowerPlant.hpp"
#include "nuclear_bits/dsl/store/DataStore.hpp"
#include "nuclear_bits/dsl/store/TypeCallbackStore.hpp"

namespace NUClear {
namespace dsl {
    namespace word {
        namespace emit {

            /**
             * @brief Direct emits execute the tasks created by emitting this type immediately.
             *
             * @details When data is emitted directly, the currently executing taks is paused and the tasks that
             *          are created by this emit are execued one at a time sequentially using the current thread.
             *          This type of emit will always work even when the system is in Shutdown or before the system
             *          has started up to the main phase.
             *
             * @param data the data to emit
             *
             * @tparam DataType the datatype that is being emitted
             */
            template <typename DataType>
            struct Direct {

                static void emit(PowerPlant& powerplant, std::shared_ptr<DataType> data) {

                    // Set our data in the store
                    store::DataStore<DataType>::set(data);

                    for (auto& reaction : store::TypeCallbackStore<DataType>::get()) {
                        try {
                            auto task = reaction->get_task();
                            if (task) {
                                task = task->run(std::move(task));
                            }
                        }
                        catch (const std::exception& ex) {
                            powerplant.log<NUClear::ERROR>("There was an exception while generating a reaction",
                                                           ex.what());
                        }
                        catch (...) {
                            powerplant.log<NUClear::ERROR>(
                                "There was an unknown exception while generating a reaction");
                        }
                    }
                }
            };

        }  // namespace emit
    }      // namespace word
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_WORD_EMIT_DIRECT_HPP
