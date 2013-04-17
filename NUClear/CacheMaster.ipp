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
    std::vector<std::shared_ptr<TData>> ReactorController::CacheMaster::Cache<TData>::m_cache = std::vector<std::shared_ptr<TData>>();
    
    template <typename TData>
    void ReactorController::CacheMaster::Cache<TData>::capacity(int num) {
        m_capacity = std::max(m_capacity, num);
    }
    
    template <typename TData>
    void ReactorController::CacheMaster::Cache<TData>::cache(TData *data) {
        m_cache.push_back(std::shared_ptr<TData>(data));
    }
    
    template <typename TData>
    std::shared_ptr<TData> ReactorController::CacheMaster::Cache<TData>::get() {
        return m_cache[0];
    }
    
    template <typename TData>
    std::vector<std::shared_ptr<TData>> ReactorController::CacheMaster::Cache<TData>::get(int length) {
        
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
    std::shared_ptr<std::vector<std::shared_ptr<const TData>>> ReactorController::CacheMaster::getData(Internal::Last<num, TData>*) {
        
        // TODO get the last n data items somehow
        
        auto d = new std::vector<std::shared_ptr<const TData>>();
        return std::shared_ptr<std::vector<std::shared_ptr<const TData>>>(d);
    }
    
    // == Private Methods ==
    template <typename TData>
    void ReactorController::CacheMaster::cache(TData* data) {
        Cache<TData>::cache(data);
    }
}
