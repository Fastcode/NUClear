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

#ifndef NUCLEAR_INTERNAL_EXTENSIONS_LINKED_H
#define NUCLEAR_INTERNAL_EXTENSIONS_LINKED_H

#include "NUClear/NUClear.h"

template <typename TData, int index>
struct NUClear::PowerPlant::CacheMaster::Get<NUClear::Internal::CommandTypes::Linked<TData, index>> {
    Internal::CommandTypes::Linked<TData, index> operator()() {
        return Internal::CommandTypes::Linked<TData, index>();
    }
};

template <typename... TData, typename TElement, int index>
// TODO this needs to be a struct!
std::shared_ptr<TElement> NUClear::PowerPlant::CacheMaster::doFill(std::tuple<TData...> data, Internal::CommandTypes::Linked<TElement, index>) {
    
    // Build our queue we are using for our search
    std::deque<void*> searchQueue;
    
    // Push on our first element
    searchQueue.push_front(static_cast<void*>(std::get<index>(data).get()));
    
    while (!searchQueue.empty()) {
        auto el = searchQueue.front();
        searchQueue.pop_front();
        
        auto it = m_linkedCache.find(el);
        
        if(it != std::end(m_linkedCache)) {
            
            for(auto& element : it->second) {
                if(element.first == typeid(std::shared_ptr<TElement>)) {
                    return std::static_pointer_cast<TElement>(element.second);
                }
                else {
                    searchQueue.push_back(element.second.get());
                }
            }
        }
    }
    
    // We don't have the data
    throw Internal::Magic::NoDataException();
}

#endif
