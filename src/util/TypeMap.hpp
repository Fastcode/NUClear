/*
 * MIT License
 *
 * Copyright (c) 2013 NUClear Contributors
 *
 * This file is part of the NUClear codebase.
 * See https://github.com/Fastcode/NUClear for further info.
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

#ifndef NUCLEAR_UTIL_TYPEMAP_HPP
#define NUCLEAR_UTIL_TYPEMAP_HPP

#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

namespace NUClear {
namespace util {

    /**
     * The simplest and fastest map format, It stores a single value and returns it when requested later.
     *
     * This map stores a single value in it's store when the set function is called, and when get is later called this
     * object will be returned.
     * This map is accessed by template parameters, because of this when the compiler compiles this map.
     * It can resolve each of the map accesses into a direct function call.
     * This allows the map to be looked up at compile time and optimized to very efficient code.
     * There are several variations of the Map provided through the MapType parameter the operation of each of these is
     * described in their individual documentation.
     *
     * @attention
     *  Note that because this is an entirely static class, if two maps with the same MapID are used, they access the
     *  same map
     */
    template <typename MapID, typename Key, typename Value>
    class TypeMap {
    public:
        /// Deleted rule-of-five as this class is a static class.
        TypeMap()                                       = delete;
        virtual ~TypeMap()                              = delete;
        TypeMap(const TypeMap& /*other*/)               = delete;
        TypeMap(TypeMap&& /*other*/) noexcept           = delete;
        TypeMap operator=(const TypeMap& /*other*/)     = delete;
        TypeMap operator=(TypeMap&& /*other*/) noexcept = delete;

    private:
        /// The data variable where the data is stored for this map key.
        static std::shared_ptr<Value> data;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

    public:
        /**
         * Stores the passed value in this map.
         *
         * @param d A pointer to the data to be stored (the map takes ownership)
         */
        static void set(std::shared_ptr<Value> d) {
            std::atomic_store_explicit(&data, std::move(d), std::memory_order_release);
        }

        /**
         * Gets the value that was previously stored.
         *
         * @return A shared_ptr to the data that was previously stored
         */
        static std::shared_ptr<Value> get() {
            return std::atomic_load_explicit(&data, std::memory_order_acquire);
        }
    };

    /// Initialize our shared_ptr data
    template <typename MapID, typename Key, typename Value>
    std::shared_ptr<Value>
        TypeMap<MapID, Key, Value>::data;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

}  // namespace util
}  // namespace NUClear

#endif  // NUCLEAR_UTIL_TYPEMAP_HPP
