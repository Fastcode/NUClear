/*
 * MIT License
 *
 * Copyright (c) 2015 NUClear Contributors
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

#ifndef NUCLEAR_UTIL_SERIALISE_SERIALISE_HPP
#define NUCLEAR_UTIL_SERIALISE_SERIALISE_HPP

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <vector>

#include "../demangle.hpp"
#include "xxhash.hpp"

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

        // Trivially copyable data
        template <typename T>
        struct Serialise<T, std::enable_if_t<std::is_trivially_copyable<T>::value, T>> {

            static std::vector<uint8_t> serialise(const T& in) {
                std::vector<uint8_t> out(sizeof(T));
                std::memcpy(out.data(), &in, sizeof(T));
                return out;
            }

            static T deserialise(const std::vector<uint8_t>& in) {
                if (in.size() != sizeof(T)) {
                    throw std::length_error("Serialised data is not the correct size");
                }
                return *reinterpret_cast<const T*>(in.data());
            }

            static uint64_t hash() {

                // Serialise based on the demangled class name
                const std::string type_name = demangle(typeid(T).name());
                return xxhash64(type_name.c_str(), type_name.size(), 0x4e55436c);
            }
        };

        // Iterable of trivially copyable data
        template <typename T>
        using iterator_value_type_t =
            typename std::iterator_traits<decltype(std::begin(std::declval<T>()))>::value_type;
        template <typename T>
        struct Serialise<T, std::enable_if_t<std::is_trivially_copyable<iterator_value_type_t<T>>::value, T>> {

            using V = std::remove_reference_t<iterator_value_type_t<T>>;

            static std::vector<uint8_t> serialise(const T& in) {
                std::vector<uint8_t> out;
                out.reserve(sizeof(V) * size_t(std::distance(std::begin(in), std::end(in))));

                for (const V& item : in) {
                    const char* i = reinterpret_cast<const char*>(&item);
                    out.insert(out.end(), i, i + sizeof(decltype(item)));
                }

                return out;
            }

            static T deserialise(const std::vector<uint8_t>& in) {

                if (in.size() % sizeof(V) != 0) {
                    throw std::length_error("Serialised data is not the correct size");
                }

                T out;

                const auto* data = reinterpret_cast<const V*>(in.data());

                out.insert(out.end(), data, data + (in.size() / sizeof(V)));

                return out;
            }

            static uint64_t hash() {

                // Serialise based on the demangled class name
                std::string type_name = demangle(typeid(T).name());
                return xxhash64(type_name.c_str(), type_name.size(), 0x4e55436c);
            }
        };

        // Google protobuf
        template <typename T>
        struct Serialise<T,
                         std::enable_if_t<std::is_base_of<::google::protobuf::Message, T>::value
                                              || std::is_base_of<::google::protobuf::MessageLite, T>::value,
                                          T>> {

            static std::vector<uint8_t> serialise(const T& in) {
                std::vector<uint8_t> output(in.ByteSize());
                in.SerializeToArray(output.data(), output.size());

                return output;
            }

            static T deserialise(const std::vector<uint8_t>& in) {
                // Make a buffer
                T out;

                // Deserialize it
                out.ParseFromArray(in.data(), in.size());
                return out;
            }

            static uint64_t hash() {

                // We have to construct an instance to call the reflection functions
                T type;
                // We base the hash on the name of the protocol buffer
                return xxhash64(type.GetTypeName().c_str(), type.GetTypeName().size(), 0x4e55436c);
            }
        };

    }  // namespace serialise
}  // namespace util
}  // namespace NUClear

#endif  // NUCLEAR_UTIL_SERIALISE_SERIALISE_HPP
