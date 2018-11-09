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

#ifndef NUCLEAR_DSL_STORE_THREADSTORE_HPP
#define NUCLEAR_DSL_STORE_THREADSTORE_HPP

#include "../../util/platform.hpp"

namespace NUClear {
namespace dsl {
    namespace store {

        /**
         * @brief Thread local datastore used for out of band communication.
         *
         * @details In NUClear, there is a disjoint that must exist between reactions which are opaque and the
         *          internals of these reactions which are strongly typed to ensure speed. This creates a problem
         *          where handlers for reactions want to pass data to them but cannot directly access them. This
         *          datastore is used in this case to provide thread local storage that is typed. Using this
         *          variable a handler can store a stack variable it has and when it generates a ReactionTask as
         *          that happens on the same thread, the get functions inside can access this same variable to
         *          bypass the opaque barrier between the handler and the reaction.
         *
         * @tparam DataType the datatype that is to be stored
         * @tparam Index    an optional index that can be used in the event that multiple variables of the same type
         *                  are needed
         */
        template <typename DataType, int Index = 0>
        struct ThreadStore {
            static ATTRIBUTE_TLS DataType* value;
        };

        template <typename DataType, int index>
        ATTRIBUTE_TLS DataType* ThreadStore<DataType, index>::value = nullptr;

    }  // namespace store
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_STORE_THREADSTORE_HPP
