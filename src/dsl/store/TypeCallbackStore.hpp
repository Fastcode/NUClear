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

#ifndef NUCLEAR_DSL_STORE_TYPECALLBACKSTORE_HPP
#define NUCLEAR_DSL_STORE_TYPECALLBACKSTORE_HPP

#include <memory>
#include "../../threading/Reaction.hpp"
#include "../../util/TypeList.hpp"

namespace NUClear {
namespace dsl {
    namespace store {

        /**
         * @biref The main callback store for reactions that are executed when a paticular type is emitted.
         *
         * @details This store is the main location that callbacks that are executed on type are stored.
         *          This method of storing is how the system realises compile time message dispatch. By storing each
         *          differnt type of message user in its own location the system knows exactly which reactions to
         *          execuing without having to do an expensive lookup. This reduces the latency and computational
         *          power invovled in spawning a new reaction when a type is emitted.
         *
         * @tparam TriggeringType the type that when emitted will start this function
         */
        template <typename TriggeringType>
        using TypeCallbackStore = util::TypeList<TriggeringType, TriggeringType, std::shared_ptr<threading::Reaction>>;

    }  // namespace store
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_STORE_TYPECALLBACKSTORE_HPP
