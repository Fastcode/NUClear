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

#ifndef NUCLEAR_UTIL_GENERATE_CALLBACK_H
#define NUCLEAR_UTIL_GENERATE_CALLBACK_H

#include "nuclear_bits/util/apply.h"
#include "nuclear_bits/util/RelevantArguments.h"
#include "nuclear_bits/util/exception/CancelRunException.h"

namespace NUClear {
    namespace util {
        
        template <size_t I = 0, typename... TData>
        inline typename std::enable_if<I == sizeof...(TData), bool>::type
        checkData(const std::tuple<TData...>&) {
            return true;
        }
        
        template<size_t I = 0, typename... TData>
        inline typename std::enable_if<I < sizeof...(TData), bool>::type
        checkData(const std::tuple<TData...>& t) {
            return std::get<I>(t) && checkData<I + 1>(t);
        }
        
        template <typename DSL, typename TFunc>
        std::function<std::function<void ()> (threading::ReactionTask&)> generate_callback(TFunc&& callback) {
            // Make our callback generator
            return [callback] (threading::ReactionTask& r) {
                
                // Bind our data to a variable (get in original thread)
                auto data = DSL::get(r);
                
                // Check if our data is good (all the data exists) otherwise terminate the call
                if(!checkData(data)) {
                    throw CancelRunException();
                }
                
                // Execute with the stored data
                return [callback, data] {
                    // We call with only the relevant arguments to the passed function
                    util::apply(callback, std::move(data), Meta::Do<util::RelevantArguments<TFunc, Meta::Do<util::DereferenceTuple<decltype(data)>>>>());
                };
            };
        }
    }
}

#endif