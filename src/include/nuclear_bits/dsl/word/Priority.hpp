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

#ifndef NUCLEAR_DSL_WORD_PRIORITY_H
#define NUCLEAR_DSL_WORD_PRIORITY_H

#include "nuclear_bits/dsl/operation/TypeBind.hpp"

namespace NUClear {
    namespace dsl {
        namespace word {

            struct Priority {
                
                struct HIGH {
                    /// High priority runs with 1000 value
                    static constexpr int value = 1000;
                    
                    template <typename DSL>
                    static inline int priority(threading::Reaction&) {
                        return value;
                    }
                };
                
                struct NORMAL {
                    /// Normal priority runs with 500 value
                    static constexpr int value = 500;
                    
                    template <typename DSL>
                    static inline int priority(threading::Reaction&) {
                        return value;
                    }
                };
                
                struct LOW {
                    /// Low priority runs with 100 value
                    static constexpr int value = 100;
                    
                    template <typename DSL>
                    static inline int priority(threading::Reaction&) {
                        return value;
                    }
                };
            };
        }
    }
}

#endif
