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

#ifndef NUCLEAR_DSL_WORD_EMIT_LOCAL_HPP
#define NUCLEAR_DSL_WORD_EMIT_LOCAL_HPP

#include "../../../PowerPlant.hpp"
#include "../../../util/TypeMap.hpp"
#include "../../store/DataStore.hpp"
#include "../../store/ThreadStore.hpp"
#include "../../store/TypeCallbackStore.hpp"

namespace NUClear {
namespace dsl {
    namespace word {
        namespace emit {

            /**
             * When emitting data under this scope, tasks are distributed via the thread pool for execution.
             *
             * @code emit<Scope::LOCAL>(data, dataType); @endcode
             *
             * @attention
             *  Note that this type of emission is the default behaviour when emitting without a specified scope.
             *  @code emit(data, dataType); @endcode
             *
             * @tparam DataType The datatype of the object to emit
             *
             * @param data The data to emit
             */
            template <typename DataType>
            struct Local {

                static void emit(PowerPlant& powerplant, std::shared_ptr<DataType> data) {

                    // Run all our reactions that are interested
                    for (auto& reaction : store::TypeCallbackStore<DataType>::get()) {

                        // Set our thread local store data
                        store::ThreadStore<std::shared_ptr<DataType>>::value = &data;
                        powerplant.submit(reaction->get_task());
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

#endif  // NUCLEAR_DSL_WORD_EMIT_LOCAL_HPP
