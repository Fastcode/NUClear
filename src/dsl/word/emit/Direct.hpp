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

#ifndef NUCLEAR_DSL_WORD_EMIT_DIRECT_HPP
#define NUCLEAR_DSL_WORD_EMIT_DIRECT_HPP

#include "../../../PowerPlant.hpp"
#include "../../store/DataStore.hpp"
#include "../../store/ThreadStore.hpp"
#include "../../store/TypeCallbackStore.hpp"

namespace NUClear {
namespace dsl {
    namespace word {
        namespace emit {

            /**
             * @brief
             *  When emitting data under this scope, the tasks created as a result of this emission will bypass the
             *  threadpool, and be executed immediately.
             *
             * @details
             *  @code emit<Scope::DIRECT>(data, dataType); @endcode
             *  When data is emitted via this scope, the task which is currently executing will be paused. At this time
             *  any tasks created as a result of this emission are executed one at a time sequentially, using the
             *  current thread.  This type of emission will always run even when the system is in its Shutdown process
             *  or before the system has started up to the main phase.
             *
             * @attention
             *  This scope is useful for reactors which emit data to themselves.
             *
             * @param data
             *  the data to emit
             * @tparam DataType
             *  the datatype that is being emitted
             */
            template <typename DataType>
            struct Direct {

                static void emit(PowerPlant& powerplant, std::shared_ptr<DataType> data) {

                    // Run all our reactions that are interested
                    for (auto& reaction : store::TypeCallbackStore<DataType>::get()) {
                        try {

                            // Set our thread local store data each time (as during direct it can be overwritten)
                            store::ThreadStore<std::shared_ptr<DataType>>::value = &data;

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

                    // Unset our thread local store data
                    store::ThreadStore<std::shared_ptr<DataType>>::value = nullptr;

                    // Set the data into the global store
                    store::DataStore<DataType>::set(data);
                }
            };

        }  // namespace emit
    }      // namespace word
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_WORD_EMIT_DIRECT_HPP
