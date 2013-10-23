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

#ifndef NUCLEAR_INTERNAL_MAGIC_TYPEMAP_H
#define NUCLEAR_INTERNAL_MAGIC_TYPEMAP_H

#include <deque>
#include <vector>

namespace NUClear {
namespace Internal {
namespace Magic {
    
    /**
     * @brief This exception is thrown when there is no data in the map to return.
     */
    struct NoDataException {};
    
    /**
     * @brief The simplest and fastest map format, It stores a single value and returns it when requested later.
     *
     * @details
     *  This map stores a single value in it's store when the set function is called, and when get is later called
     *  this object will be returned. This map is accessed by template paramters, because of this when the compiler
     *  compiles this map. It can resolve each of the map accesses into a direct function call. This allows the map to
     *  be looked up at compile time and optimized to very efficent code. There are several variations of the Map
     *  provided through the MapType parameter the operation of each of these is described in their individual
     *  documentation.
     *
     * @attention
     *  Note that because this is an entirely static class, if two maps with the same TMapID are used, they access the
     *  same map
     *
     * @author Trent Houliston
     *
     * @see CompiledMap
     */
    template <typename TMapID, typename TKey, typename TValue>
    class TypeMap {
        private:
            /// @brief Deleted constructor as this class is a static class.
            TypeMap() = delete;
            /// @brief Deleted destructor as this class is a static class.
            ~TypeMap() = delete;
            /// @brief the data variable where the data is stored for this map key.
            static std::shared_ptr<TValue> data;
            
        public:
            /**
             * @brief Stores the passed value in this map.
             *
             * @param data a pointer to the data to be stored (the map takes ownership)
             */
            static void set(std::shared_ptr<TValue> data) {
                data = std::shared_ptr<TValue>(data);
            };
        
            /**
             * @brief Gets the value that was previously stored.
             *
             * @return a shared_ptr to the data that was previously stored
             *
             * @throws NoDataException if there is no data that was previously stored
             */
            static std::shared_ptr<TKey> get() {
                //If the pointer is not a nullptr
                if(data) {
                    return data;
                }
                else {
                    throw NoDataException();
                }
            }
    };
}
}
}

#endif
