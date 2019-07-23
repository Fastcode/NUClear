/*
 * Copyright (C) 2013      Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
 *               2014-2017 Trent Houliston <trent@houliston.me>
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

#ifndef NUCLEAR_UTIL_SERIALISE_SERIALISE_HPP
#define NUCLEAR_UTIL_SERIALISE_SERIALISE_HPP

#include <string>
#include <type_traits>

#include "../demangle.hpp"
#include "xxhash.h"

// Forward declare google protocol buffers
// We don't actually use or require them, but if
// Anyone ever does in their code this will work
namespace google {
namespace protobuf {
    class Message;
    class MessageLite;
}  // namespace protobuf
}  // namespace google

namespace NUClear {
namespace util {
    namespace serialise {

        template <typename T, typename Check = T>
        struct Serialise;

        // Plain old data
        template <typename T>
        struct Serialise<T, std::enable_if_t<std::is_trivial<T>::value, T>> {

            static inline std::vector<char> serialise(const T& in) {

                // Copy the bytes into an array
                const char* dataptr = reinterpret_cast<const char*>(&in);
                return std::vector<char>(dataptr, dataptr + sizeof(T));
            }

            static inline T deserialise(const std::vector<char>& in) {

                // Copy the data into an object of the correct type
                T ret = *reinterpret_cast<T*>(in.data());
                return ret;
            }

            static inline uint64_t hash() {

                // Serialise based on the demangled class name
                std::string type_name = demangle(typeid(T).name());
                return XXH64(type_name.c_str(), type_name.size(), 0x4e55436c);
            }
        };

        // Iterable of pod
        template <typename T>
        struct Serialise<
            T,
            std::enable_if_t<
                std::is_same<typename std::iterator_traits<decltype(std::declval<T>().begin())>::iterator_category,
                             std::random_access_iterator_tag>::value,
                T>> {

            using StoredType = std::remove_reference_t<decltype(*std::declval<T>().begin())>;

            static inline std::vector<char> serialise(const T& in) {
                std::vector<char> out;
                out.reserve(std::size_t(std::distance(in.begin(), in.end())));

                for (const StoredType& item : in) {
                    const char* i = reinterpret_cast<const char*>(&item);
                    out.insert(out.end(), i, i + sizeof(decltype(item)));
                }

                return out;
            }

            static inline T deserialise(const std::vector<char>& in) {

                T out;

                const StoredType* data = reinterpret_cast<const StoredType*>(in.data());

                out.insert(out.end(), data, data + (in.size() / sizeof(StoredType)));

                return out;
            }

            static inline uint64_t hash() {

                // Serialise based on the demangled class name
                std::string type_name = demangle(typeid(T).name());
                return XXH64(type_name.c_str(), type_name.size(), 0x4e55436c);
            }
        };

        // Google protobuf
        template <typename T>
        struct Serialise<T,
                         std::enable_if_t<std::is_base_of<::google::protobuf::Message, T>::value
                                              || std::is_base_of<::google::protobuf::MessageLite, T>::value,
                                          T>> {

            static inline std::vector<char> serialise(const T& in) {
                std::vector<char> output(in.ByteSize());
                in.SerializeToArray(output.data(), output.size());

                return output;
            }

            static inline T deserialise(const std::vector<char>& in) {
                // Make a buffer
                T out;

                // Deserialize it
                out.ParseFromArray(in.data(), in.size());
                return out;
            }

            static inline uint64_t hash() {

                // We have to construct an instance to call the reflection functions
                T type;
                // We base the hash on the name of the protocol buffer
                return XXH64(type.GetTypeName().c_str(), type.GetTypeName().size(), 0x4e55436c);
            }
        };

    }  // namespace serialise
}  // namespace util
}  // namespace NUClear

#endif  // NUCLEAR_UTIL_SERIALISE_SERIALISE_HPP
