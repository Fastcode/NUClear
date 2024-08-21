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

#ifndef NUCLEAR_DSL_OPERATION_CACHE_GET_HPP
#define NUCLEAR_DSL_OPERATION_CACHE_GET_HPP

#include "../store/DataStore.hpp"
#include "../store/ThreadStore.hpp"

namespace NUClear {
namespace dsl {
    namespace operation {

        /**
         * Accesses a variable from the shared data cache when used.
         *
         * NUClear maintains a datastore of the latest data emitted of each type in the system.
         * This utility type accesses this shared cache and accesses the latest data using get.
         * To use this utility inherit from this type with the DataType to listen for.
         *
         * @tparam DataType the data type that will be accessed from the cache
         */
        template <typename DataType>
        struct CacheGet {

            template <typename DSL, typename T = DataType>
            static std::shared_ptr<const T> get(const threading::ReactionTask& /*task*/) {

                return store::ThreadStore<std::shared_ptr<T>>::value == nullptr
                           ? store::DataStore<DataType>::get()
                           : *store::ThreadStore<std::shared_ptr<T>>::value;
            }
        };

    }  // namespace operation
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_OPERATION_CACHE_GET_HPP
