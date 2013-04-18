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

    template <typename TData>
    int ReactorController::CacheMaster::Cache<TData>::m_capacity = 1;
    
    template <typename TData>
    std::deque<std::shared_ptr<TData>> ReactorController::CacheMaster::Cache<TData>::m_cache = std::deque<std::shared_ptr<TData>>(1);
    
    template <typename TData>
    void ReactorController::CacheMaster::Cache<TData>::capacity(int num) {
            for(;m_capacity < num; ++m_capacity) {
                m_cache.emplace_back(nullptr);
            }
    }
    
    template <typename TData>
    void ReactorController::CacheMaster::Cache<TData>::cache(TData *data) {
        m_cache.pop_back();
        m_cache.push_front(std::shared_ptr<TData>(data));
    }
    
    template <typename TData>
    std::shared_ptr<TData> ReactorController::CacheMaster::Cache<TData>::get() {
        if(!m_cache.empty()) {
            return m_cache.front();
        }
        else {
            throw NoDataException();
        }
    }
    
    template <typename TData>
    std::shared_ptr<std::vector<std::shared_ptr<const TData>>> ReactorController::CacheMaster::Cache<TData>::get(int length) {
        return std::shared_ptr<std::vector<std::shared_ptr<const TData>>>(new std::vector<std::shared_ptr<const TData>>(std::begin(m_cache), std::begin(m_cache) + length));
    }
    
    template <int num, typename TData>
    void ReactorController::CacheMaster::ensureCache() {
        Cache<TData>::capacity(num);
    }
    
    template <typename TData>
    std::shared_ptr<TData> ReactorController::CacheMaster::getData(TData*) {
        return Cache<TData>::get();
    }
    
    template <int num, typename TData>
    std::shared_ptr<std::vector<std::shared_ptr<const TData>>> ReactorController::CacheMaster::getData(Internal::CommandTypes::Last<num, TData>*) {
        
        return Cache<TData>::get(num);
    }
    
    template <int ticks, class period>
    std::shared_ptr<std::chrono::time_point<std::chrono::steady_clock>> ReactorController::CacheMaster::getData(Internal::CommandTypes::Every<ticks, period>*) {
        
        return std::shared_ptr<std::chrono::time_point<std::chrono::steady_clock>>(new std::chrono::time_point<std::chrono::steady_clock>(Cache<Internal::CommandTypes::Every<ticks, period>>::get()->m_time));
    }
    
    template <typename TData>
    void ReactorController::CacheMaster::cache(TData* data) {
        Cache<TData>::cache(data);
    }
}
