/*
 * MIT License
 *
 * Copyright (c) 2024 NUClear Contributors
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

#ifndef NUCLEAR_DSL_FUSION_FIND_WORDS_HPP
#define NUCLEAR_DSL_FUSION_FIND_WORDS_HPP

#include "../../util/meta/Filter.hpp"

namespace NUClear {
namespace dsl {
    namespace fusion {

        /**
         * Filters the DSL words to only those that have the correct function.
         *
         * It will:
         * - Attempt to use the word directly
         * - If the word does not have the correct function, it will try the DSLProxy for that word
         * - If neither have the correct function, it will remove the word from the list
         *
         * @tparam Check The function to check for
         * @tparam Ts    The words to check
         */
        template <template <typename> class Check, typename... Ts>
        using FindWords = Filter<Check, std::conditional_t<Check<Ts>::value, Ts, operation::DSLProxy<Ts>>...>;

    }  // namespace fusion
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_FUSION_FIND_WORDS_HPP
