/*
 * Copyright (C) 2013 Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
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

#ifndef NUCLEAR_DSL_WORD_TRIGGER_H
#define NUCLEAR_DSL_WORD_TRIGGER_H

#include "nuclear_bits/dsl/operation/CacheGet.h"
#include "nuclear_bits/dsl/operation/TypeBind.h"

namespace NUClear {
    namespace dsl {
        namespace word {

            /**
             * @ingroup Wrappers
             * @brief This is a wrapper class which is used to list the data types to trigger a callback on.
             *
             * @details
             *  This class is used in the on binding to specify which data types are to be used trigger a callback. It
             *  works under and logic for multiple types. When all of the types have been emitted at least once since
             *  the last time this event was triggered, this event is triggered again.
             *
             * @tparam TTriggers the datatypes to trigger a callback on
             */
            template <typename TType>
            struct Trigger : public operation::CacheGet<TType>, operation::TypeBind<TType> {};
        }
    }
}

#endif
