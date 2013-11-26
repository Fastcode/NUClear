/**
 * Copyright (C) 2013 Jake Woods <jake.f.woods@gmail.com>, Trent Houliston <trent@houliston.me>
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

#ifndef NUCLEAR_DSL_LAST_H
#define NUCLEAR_DSL_LAST_H

namespace NUClear {
    namespace dsl {
        
        /**
         * @ingroup SmartTypes
         * @brief This is a special type which is used to get a list of the last n emissions of type TData
         *
         * @details
         *  This class is used to get the last n data events of a type which have been emitted. These are returned
         *  as const shared pointers in a vector, which can then be used to access the data. The data is returned in
         *  descending order, i.e. the most recent event is first in the result
         *
         * @tparam n        The number of events to get
         * @tparam TData    The datatype of object to get
         *
         * @return  Returns a vector of shared_ptr with the signature std::vector<std::shared_ptr<TData>>
         */
        template <int n, typename TData>
        struct Last {
            Last() = delete;
            ~Last() = delete;
        };
    }
}

#endif
