/*
 * MIT License
 *
 * Copyright (c) 2014 NUClear Contributors
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

#ifndef NUCLEAR_DSL_WORD_EMIT_INLINE_HPP
#define NUCLEAR_DSL_WORD_EMIT_INLINE_HPP

#include "../../../PowerPlant.hpp"
#include "../../store/DataStore.hpp"
#include "../../store/ThreadStore.hpp"
#include "../../store/TypeCallbackStore.hpp"

namespace NUClear {
namespace dsl {
    namespace word {
        namespace emit {

            /**
             * When emitting data under this scope, the tasks created as a result of this emission will bypass the
             * thread pool, and be executed immediately if they can be.
             * If a task specifies that it is not inlinable, it will be executed in the thread pool as normal.
             *
             * @code emit<Scope::INLINE>(data, dataType); @endcode
             * When data is emitted via this scope, the task which is currently executing will be paused.
             * At this time any tasks created as a result of this emission are executed one at a time sequentially,
             * using the current thread.
             * This type of emission will always run even when the system is in its Shutdown process or before the
             * system has started up to the main phase.
             *
             * @attention
             *  This scope is useful for reactors which emit data to themselves.
             *
             * @tparam DataType The datatype that is being emitted
             *
             * @param data The data to emit
             */
            template <typename DataType>
            struct Inline {

                static void emit(PowerPlant& powerplant, std::shared_ptr<DataType> data) {

                    // Run all our reactions that are interested
                    for (auto& reaction : store::TypeCallbackStore<DataType>::get()) {
                        // Set our thread local store data each time (as during inline it can be overwritten)
                        store::ThreadStore<std::shared_ptr<DataType>>::value = &data;
                        powerplant.submit(reaction->get_task(true));
                    }

                    // Unset our thread local store data
                    store::ThreadStore<std::shared_ptr<DataType>>::value = nullptr;

                    // Set the data into the global store
                    store::DataStore<DataType>::set(data);
                }
            };

        }  // namespace emit
    }  // namespace word
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_WORD_EMIT_INLINE_HPP
