/*
 * MIT License
 *
 * Copyright (c) 2014 NUClear Contributors
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

#ifndef NUCLEAR_UTIL_TYPELIST_HPP
#define NUCLEAR_UTIL_TYPELIST_HPP

#include <memory>
#include <mutex>
#include <vector>

namespace NUClear {
namespace util {

    template <typename MapID, typename Key, typename Value>
    class TypeList {
    public:
        /// Deleted rule-of-five as this class is a static class
        TypeList()                                        = delete;
        virtual ~TypeList()                               = delete;
        TypeList(const TypeList& /*other*/)               = delete;
        TypeList(TypeList&& /*other*/) noexcept           = delete;
        TypeList operator=(const TypeList& /*other*/)     = delete;
        TypeList operator=(TypeList&& /*other*/) noexcept = delete;

    private:
        /// The data variable where the data is stored for this map key
        static std::vector<Value> data;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

    public:
        /**
         * Gets the list that is stored in this type location.
         *
         * @return A reference to the vector stored in this location
         */
        static std::vector<Value>& get() {
            return data;
        }
    };

    /// Initialize our type list data
    template <typename MapID, typename Key, typename Value>
    std::vector<Value> TypeList<MapID, Key, Value>::data;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

}  // namespace util
}  // namespace NUClear

#endif  // NUCLEAR_UTIL_TYPELIST_HPP
