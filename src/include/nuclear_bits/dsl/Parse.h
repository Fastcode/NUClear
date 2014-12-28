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

#ifndef NUCLEAR_DSL_PARSE_H
#define NUCLEAR_DSL_PARSE_H

#include "Fusion.h"

namespace NUClear {
    namespace dsl {

        template <typename... Sentence>
        struct Parse {
            
            using DSL = Fusion<Sentence...>;
        
            // TODO SFINAE for precondition, if there isn't a precondition then we still need a function that returns true
            
            static bool precondition(threading::Reaction& r) {
                return DSL::template precondition<Parse<Sentence...>>(r);
            }
            
            // TODO SFINAE for postcondition, if there isn't a postconditoin then we still need an empty function
            
            static void postcondition(threading::ReactionTask& r) {
                DSL::template postcondition<Parse<Sentence...>>(r);
            }
            
            
            // TODO SFINAE for get, if there isn't a get function then we still need an empty tuple
            
            
            static auto get(threading::ReactionTask& r)
            -> decltype(DSL::template get<Parse<Sentence...>>(r)) {
                return DSL::template get<Parse<Sentence...>>(r);
            }
            
            // TODO for bind we always need to have a bind function (at least one) so have a static assert that if there isn't one then cry a lot
            
            template <typename TFunc>
            static auto bind(Reactor& r, const std::string& label, TFunc callback)
            -> decltype(DSL::template bind<Parse<Sentence...>>(r, label, std::forward<TFunc>(callback))) {
                return DSL::template bind<Parse<Sentence...>>(r, label, std::forward<TFunc>(callback));
            }
        
        };
    }
}

#endif
