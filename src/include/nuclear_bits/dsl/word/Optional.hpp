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

#ifndef NUCLEAR_DSL_WORD_OPTIONAL_H
#define NUCLEAR_DSL_WORD_OPTIONAL_H

namespace NUClear {
    namespace dsl {
        namespace word {
            
            template <typename TData>
            struct OptionalWrapper {
                
                OptionalWrapper(TData&& d) : d(std::forward<TData>(d)) {}
                
                TData operator*() const {
                    return std::move(d);
                }
                
                operator bool() const {
                    return true;
                }
                
                TData d;
            };

            template <typename... DSLWords>
            struct Optional : public Fusion<DSLWords...> {
                
            private:
                template <typename... TData, int... Index>
                static inline auto wrap(std::tuple<TData...>&& data, util::Sequence<Index...>)
                -> decltype(std::make_tuple(OptionalWrapper<TData>(std::move(std::get<Index>(data)))...)) {
                    return std::make_tuple(OptionalWrapper<TData>(std::move(std::get<Index>(data)))...);
                }
                
            public:
                template <typename DSL>
                static inline auto get(threading::ReactionTask& r)
                -> decltype(wrap(Fusion<DSLWords...>::template get<DSL>(r), util::GenerateSequence<0, std::tuple_size<decltype(Fusion<DSLWords...>::template get<DSL>(r))>::value>())) {
                    
                    // Wrap all of our data in optional wrappers
                    return wrap(Fusion<DSLWords...>::template get<DSL>(r), util::GenerateSequence<0, std::tuple_size<decltype(Fusion<DSLWords...>::template get<DSL>(r))>::value>());
                }
            };
        }
    }
}

#endif
