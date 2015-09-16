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

#ifndef NUCLEAR_DSL_WORD_NETWORK_H
#define NUCLEAR_DSL_WORD_NETWORK_H

namespace NUClear {
    namespace dsl {
        namespace word {

            template <typename TData>
            struct Network {
                
                struct Source {
                    // TODO have an IP and stuff in here
                };
            
                template <typename DSL, typename TFunc>
                static inline threading::ReactionHandle bind(Reactor&, const std::string&, TFunc&&) {
                    
                    std::cout << "TODO NUClear networking not yet implemented" << std::endl;
   
                    threading::ReactionHandle handle;
                    return handle;
                }
                
                template <typename DSL>
                static inline std::tuple<Source, std::shared_ptr<const TData>> get(threading::ReactionTask&) {
                    
                    return std::make_tuple(Source(), std::make_shared<TData>());
                }
                
            };
        }
    }
}

#endif
