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

#ifndef NUCLEAR_DSL_WORD_WITH_HPP
#define NUCLEAR_DSL_WORD_WITH_HPP

#include "../Fusion.hpp"
#include "../operation/CacheGet.hpp"

namespace NUClear {
namespace dsl {
    namespace word {

        /**
         * @brief
         *  This is used to define any extra data which should be provided to a subscribing a reaction.
         *
         * @details
         *  @code on<With<T2>>() @endcode
         *  Note that during runtime, the emission of data using this word will not trigger a reaction within the
         *  system.  For best use, this word should be fused with at least one other binding DSL word.
         *
         *  @code on<Trigger<T1>, With<T2>>() @endcode
         *  In the example above, when T2 is emitted into the system, it will <b>not</b> trigger a callback to the
         *  triggering reaction. Yet when T1 is emitted into the system, read-only access to the most recent copy of
         *  both T1 and T2 will be provided via a callback to the reaction.
         *
         *  If a copy of T2 is not present when T1 is emitted into the system, the task will be dropped (i.e the
         *  reaction will not run). To override this functionality, include the DSL keyword "Optional" in the request.
         *  For example:
         *  @code on<Trigger<T1>, Optional<With<T2>>>() @endcode
         *
         * @par Implements
         *  Get
         *
         * @tparam T
         *  the datatype/s which will be provided to a subscribing reaction when the reaction is triggered.
         */
        template <typename... T>
        struct With : public Fusion<operation::CacheGet<T>...> {};

    }  // namespace word
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_WORD_WITH_HPP
