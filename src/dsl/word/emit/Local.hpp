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
             * @brief
             *  When emitting data under this scope, tasks are distributed via the thread pool for execution.
             *
             * @details
             *  @code emit<Scope::LOCAL>(data, dataType); @endcode
             *
             * @attention
             *  Note that this type of emission is the default behaviour when emitting without a specified scope.
             *  @code emit(data, dataType); @endcode
             *
             * @param data
             *  the data to emit
             * @tparam DataType
             *  the datatype of the object to emit
             */
            template <typename DataType>
            struct Local {

                static void emit(PowerPlant& powerplant, std::shared_ptr<DataType> data) {

                    // Set our thread local store data
                    store::ThreadStore<std::shared_ptr<DataType>>::value = &data;

                    // Run all our reactions that are interested
                    for (auto& reaction : store::TypeCallbackStore<DataType>::get()) {
                        try {
                            auto task = reaction->get_task();
                            if (task) { powerplant.submit(std::move(task)); }
                        }
                        // If there is an exception while generating a reaction print it here, this shouldn't happen
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

#endif  // NUCLEAR_DSL_WORD_EMIT_LOCAL_HPP
