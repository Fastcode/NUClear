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

#ifndef NUCLEAR_DSL_OPERATION_TYPEBIND_H
#define NUCLEAR_DSL_OPERATION_TYPEBIND_H

#include "nuclear_bits/dsl/store/TypeCallbackStore.h"
#include "nuclear_bits/util/generate_callback.h"
#include "nuclear_bits/util/get_identifier.h"

namespace NUClear {
    namespace dsl {
        namespace operation {

            template <typename TType>
            struct TypeBind {

                template <typename DSL, typename TFunc>
                static inline threading::ReactionHandle bind(Reactor&, const std::string& label, TFunc&& callback) {
                    
                    // Generate our task
                    auto task = util::generate_callback<DSL>(std::forward<TFunc>(callback));
                    
                    // Our unbinder to remove this reaction
                    auto unbinder = [] (threading::Reaction& r) {
                        
                        auto& vec = store::TypeCallbackStore<TType>::get();
                        
                        auto item = std::find_if(std::begin(vec), std::end(vec), [&r] (const std::unique_ptr<threading::Reaction>& item) {
                            return item->reactionId == r.reactionId;
                        });
                        
                        // If the item is in the list erase the item
                        if(item != std::end(vec)) {
                            vec.erase(item);
                        }
                    };
                    
                    // Get our identifier string
                    std::vector<std::string> identifier = util::get_identifier<typename DSL::DSL, TFunc>(label);
                    
                    auto reaction = std::make_unique<threading::Reaction>(identifier, task, DSL::precondition, DSL::priority, DSL::postcondition, unbinder);
                    threading::ReactionHandle handle(reaction.get());
                    
                    // Create our reaction and store it in the TypeCallbackStore
                    store::TypeCallbackStore<TType>::get().push_back(std::move(reaction));
                    
                    // Return our handle
                    return handle;
                }
            };
        }
    }
}

#endif
