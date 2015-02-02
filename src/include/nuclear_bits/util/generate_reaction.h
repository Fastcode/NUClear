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

#ifndef NUCLEAR_UTIL_GENERATE_REACTION_H
#define NUCLEAR_UTIL_GENERATE_REACTION_H

#include "nuclear_bits/dsl/word/emit/Direct.h"
#include "nuclear_bits/threading/Reaction.h"
#include "nuclear_bits/util/generate_callback.h"
#include "nuclear_bits/util/get_identifier.h"

namespace NUClear {
    namespace util {
        
        template <typename DSL, typename TUnbind, typename TFunc>
        std::unique_ptr<threading::Reaction> generate_reaction(Reactor& reactor, const std::string& label, TFunc&& callback, std::function<void(threading::Reaction&)> unbind = std::function<void(threading::Reaction&)>()) {
        
            // Make our callback generator
            auto task = util::generate_callback<DSL>(std::forward<TFunc>(callback));
            
            // Get our identifier string
            std::vector<std::string> identifier = util::get_identifier<typename DSL::DSL, TFunc>(label);
            
            // Get our powerplant
            auto& powerplant = reactor.powerplant;
            
            // Create our unbinder
            auto unbinder = [&powerplant, unbind] (threading::Reaction& r) {
                powerplant.emit<dsl::word::emit::Direct>(std::make_unique<dsl::operation::Unbind<TUnbind>>(r.reactionId));
                if(unbind) {
                    unbind(r);
                }
            };
                        
            // Create our reaction
            return std::make_unique<threading::Reaction>(identifier, task, DSL::precondition, DSL::priority, DSL::postcondition, unbinder);
            
        }
    }
}

#endif