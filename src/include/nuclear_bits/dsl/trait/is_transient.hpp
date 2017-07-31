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

#ifndef NUCLEAR_DSL_TRAIT_ISTRANSIENT_HPP
#define NUCLEAR_DSL_TRAIT_ISTRANSIENT_HPP

namespace NUClear {
namespace dsl {
    namespace trait {

        /**
         * @brief Indicates that a type is transient in the context of data availablity
         *
         * @details Often when extending the get dsl attachment point, data from that get is only available when
         *          that get is run in specific circumstances such as from a ThreadStore.
         *          When this trait is true, Reactors handle this data being unavaible differently.
         *          They will instead cache the last copy of the data that was provided and if no new data
         *          comes from the get function, they will instead provide this cached data.
         *
         * @see NUClear::dsl::store::ThreadStore
         *
         * @tparam typename the datatype that is to be considered transient
         */
        template <typename DataType>
        struct is_transient : public std::false_type {};

    }  // namespace trait
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_TRAIT_ISTRANSIENT_HPP
