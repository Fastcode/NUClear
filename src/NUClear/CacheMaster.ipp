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
    struct PowerPlant::CacheMaster::Get {
        std::shared_ptr<TData> operator()() {
            return ValueCache<TData>::get();
        }
    };
    
    template <typename TData, typename... TList>
    struct PowerPlant::CacheMaster::Fill {
        CacheMaster* context;
        Fill(CacheMaster* context) : context(context) {};
        TData operator()(TData element, std::tuple<TList...> list) {
            return element;
        }
    };

    template <typename TData>
    void PowerPlant::CacheMaster::cache(TData* data) {
        ValueCache<TData>::set(std::shared_ptr<TData>(data, [this] (TData* data) {
            if(m_linkedCache.find(data) != std::end(m_linkedCache)) {
                m_linkedCache.erase(data);
            }
            delete data;
        }));
    }
}
