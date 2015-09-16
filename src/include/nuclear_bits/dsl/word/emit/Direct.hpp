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

#ifndef NUCLEAR_DSL_WORD_EMIT_DIRECT_H
#define NUCLEAR_DSL_WORD_EMIT_DIRECT_H

#include "nuclear_bits/PowerPlant.hpp"
#include "nuclear_bits/dsl/store/DataStore.hpp"
#include "nuclear_bits/dsl/store/TypeCallbackStore.hpp"

namespace NUClear {
    namespace dsl {
        namespace word {
            namespace emit {

                template <typename TData>
                struct Direct {

                    static void emit(PowerPlant&, std::shared_ptr<TData> data) {
                        
                        // Set our data in the store
                        store::DataStore<TData>::set(data);
                        
                        for(auto& reaction : store::TypeCallbackStore<TData>::get()) {
                            try {
                                auto task = reaction->getTask();
                                task = task->run(std::move(task));
                            }
                            catch(util::CancelRunException ex) {
                            }
                            catch(...) {
                                // TODO should something happen here?
                            }
                        }
                    }
                };
            }
        }
    }
}

#endif
