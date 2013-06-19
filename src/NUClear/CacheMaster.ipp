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


namespace NUClear {
    
    template <int num, typename TData>
    void PowerPlant::CacheMaster::ensureCache() {
        ValueCache<TData>::cache::minCapacity(num);
    }
    
    template <typename TData>
    std::shared_ptr<TData> PowerPlant::CacheMaster::getData(TData*) {
        return ValueCache<TData>::get();
    }
    
    template <int num, typename TData>
    std::shared_ptr<std::vector<std::shared_ptr<const TData>>> PowerPlant::CacheMaster::getData(Internal::CommandTypes::Last<num, TData>*) {
        return ValueCache<TData>::cache::get(num);
    }
    
    template <int ticks, class period>
    std::shared_ptr<std::chrono::time_point<std::chrono::steady_clock>> PowerPlant::CacheMaster::getData(Internal::CommandTypes::Every<ticks, period>*) {
        
        return std::shared_ptr<std::chrono::time_point<std::chrono::steady_clock>>(new std::chrono::time_point<std::chrono::steady_clock>(ValueCache<Internal::CommandTypes::Every<ticks, period>>::get()->m_time));
    }
    
    template <typename TData, int index>
    Internal::CommandTypes::Linked<TData, index> PowerPlant::CacheMaster::getData(Internal::CommandTypes::Linked<TData, index>*) {
        return Internal::CommandTypes::Linked<TData, index>();
    }
    
    template <typename TData>
    void PowerPlant::CacheMaster::cache(TData* data) {
        ValueCache<TData>::set(std::shared_ptr<TData>(data, [this] (TData* data) {
            if(m_linkedCache.find(data) != std::end(m_linkedCache)) {
                m_linkedCache.erase(data);
            }
            delete data;
        }));
    }
    
    template <typename... TData, typename TElement>
    TElement PowerPlant::CacheMaster::doFill(std::tuple<TData...> data, TElement element) {
        return element;
    }
    
    template <typename... TData, typename TElement, int index>
    std::shared_ptr<TElement> PowerPlant::CacheMaster::doFill(std::tuple<TData...> data, Internal::CommandTypes::Linked<TElement, index>) {
        
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
}
