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

#ifndef NUCLEAR_DSL_WORD_TRIGGER_HPP
#define NUCLEAR_DSL_WORD_TRIGGER_HPP

#include "../Fusion.hpp"
#include "../operation/CacheGet.hpp"
#include "../operation/TypeBind.hpp"

namespace NUClear {
namespace dsl {
    namespace word {

        /**
         * @brief
         *  This is used to request any data dependent reactions in the system.
         *
         * @details
         *  @code on<Trigger<T>>() @endcode
         *  This will enact the execution of a task whenever T is emitted into the system. When this occurs, read-only
         *  access to T will be provided to the triggering unit via a callback.
         *
         *  @code on<Trigger<T1, T2, ... >>() @endcode
         *  Note that a this request can handle triggers on multiple types. When using multiple types in the request,
         *  the reaction will only be triggered once <b>all</b> of the trigger types have been emitted (at least once)
         *  since the last occurrence of the event.
         *
         * @par Implements
         *  Bind, Get
         *
         * @tparam Ts
         *  The datatype on which a reaction callback will be triggered.  Emission of this datatype into the system
         *  will trigger the subscribing reaction.
         */
        template <typename... Ts>
        struct Trigger : public Fusion<operation::TypeBind<Ts>..., operation::CacheGet<Ts>...> {};

    }  // namespace word
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_WORD_TRIGGER_HPP
