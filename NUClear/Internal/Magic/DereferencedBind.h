/**
 * Copyright (C) 2013 Jake Woods <jake.f.woods@gmail.com>, Trent Houliston <trent@houliston.me>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software
 * is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
 * IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef NUCLEAR_INTERNAL_MAGIC_DEREFERENCEDBIND_H
#define NUCLEAR_INTERNAL_MAGIC_DEREFERENCEDBIND_H

#include <tuple>
#include "Sequence.h"

namespace NUClear {
namespace Internal {
namespace Magic {
    namespace {
        template <typename TFunc, typename... TTypes>
        class DereferencedBind {
            private:
                std::tuple<TTypes...> params;
                TFunc callback;
            
                template<int... S>
                void apply(Sequence<S...>) {
                    callback((*(std::get<S>(params)))...);
                }
            public:
                DereferencedBind(TFunc func, TTypes... args) : params(args...), callback(func) { }
                void operator()() {
                    apply(typename GenerateSequence<sizeof...(TTypes)>::type());
                }

        };
    }
    
    template <typename TFunc, typename... TTypes>
    DereferencedBind<TFunc, TTypes...> apply(TFunc function, TTypes... args) {
        return DereferencedBind<TFunc, TTypes...>(function, args...);
    }
}
}
}

#endif
