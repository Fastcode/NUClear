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

#ifndef NUCLEAR_DSL_STORE_DATASTORE_HPP
#define NUCLEAR_DSL_STORE_DATASTORE_HPP

#include <memory>

#include "../../util/TypeMap.hpp"

namespace NUClear {
namespace dsl {
    namespace store {

        /**
         * @brief The main datastore used in the system.
         *
         * @details This datastore is the main one used by the system. When data is emitted it is stored in this
         *          typed datastore. This allows constant time access to any datatype without having to look it up.
         *          This is possible as the exact location of the store is known at compile time.
         *
         * @tparam DataType the type of data stored in this paticular datastore location
         */
        template <typename DataType>
        using DataStore = util::TypeMap<DataType, DataType, DataType>;

    }  // namespace store
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_STORE_DATASTORE_HPP
