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
     * To future me and others who look at this code.
     * You would think that you should use an atomic shared pointer rather than a mutex protected shared pointer.
     * That would make sense, then you would potentially have a lock free implementation!
     *
     * However the implementation as seen in libc++ and libstdc++ is to use a mutex protected shared pointer anyway.
     * But worse than that, it just uses a hashmap of mutexes to protect the shared pointers.
     * Specifically it looks like they just have 0xF mutexes and they hash the pointer addresses to pick one.
     * Having only a few mutexes for the entire map is a terrible idea and causes a lot of contention.
     * This is strictly worse than the already separated mutex per type that is achieved by this implementation.
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
        /// The mutex that protects the data variable
        static std::mutex data_mutex;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

    public:
        /**
         * Stores the passed value in this map.
         *
         * @param d A pointer to the data to be stored (the map takes ownership)
         */
        static void set(std::shared_ptr<Value> d) {
            const std::lock_guard<std::mutex> lock(data_mutex);
            data = d;
        }

        /**
         * Gets the value that was previously stored.
         *
         * @return A shared_ptr to the data that was previously stored
         */
        static std::shared_ptr<Value> get() {
            const std::lock_guard<std::mutex> lock(data_mutex);
            return data;
        }
    };

    /// Initialize our shared_ptr data
    template <typename MapID, typename Key, typename Value>
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
    std::shared_ptr<Value> TypeMap<MapID, Key, Value>::data;
    template <typename MapID, typename Key, typename Value>
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
    std::mutex TypeMap<MapID, Key, Value>::data_mutex;

}  // namespace util
}  // namespace NUClear

#endif  // NUCLEAR_UTIL_TYPEMAP_HPP
