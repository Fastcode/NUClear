/*
 * MIT License
 *
 * Copyright (c) 2024 NUClear Contributors
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

#ifndef NUCLEAR_EXTENSION_TRACE_PROTOBUF_HPP
#define NUCLEAR_EXTENSION_TRACE_PROTOBUF_HPP

#include <cstdint>
#include <type_traits>
#include <vector>

#include "../../util/unpack.hpp"

namespace NUClear {
namespace extension {
    namespace trace {
        namespace protobuf {

            namespace encode {
                template <typename T, typename OutputIterator, std::enable_if_t<std::is_integral<T>::value, int> = 0>
                OutputIterator redundant_varint(const T& value, const size_t& n_bytes, OutputIterator out) {
                    // Encode the value as a varint
                    uint32_t v = value;
                    for (unsigned i = 0; i < n_bytes - 1; i++) {
                        *out++ = (v & 0x7F) | 0x80;
                        v >>= 7;
                    }
                    *out++ = v & 0x7F;
                    return out;
                }

                template <typename T, typename OutputIterator, std::enable_if_t<std::is_integral<T>::value, int> = 0>
                OutputIterator varint(const T& value, OutputIterator out) {
                    // Cast the value down to its unsigned type and encode it as a varint
                    std::make_unsigned_t<T> v(value);
                    while (v >= 0x80) {
                        *out++ = (v & 0x7F) | 0x80;
                        v >>= 7;
                    }
                    *out++ = v;
                    return out;
                }

                template <typename T, typename OutputIterator, std::enable_if_t<std::is_integral<T>::value, int> = 0>
                OutputIterator fixed(const T& value, OutputIterator out) {
                    // Cast the value down to its unsigned type and encode it as fixed
                    std::make_unsigned_t<T> v(value);
                    for (size_t i = 0; i < sizeof(T); ++i) {
                        *out++ = (v >> (i * 8)) & 0xFF;
                    }
                    return out;
                }

                template <typename Iterator, typename OutputIterator>
                OutputIterator length_delimited(Iterator begin, Iterator end, OutputIterator out) {
                    // Encode the value as a varint length and then the string
                    out = varint(uint32_t(std::distance(begin, end)), out);
                    out = std::copy(begin, end, out);
                    return out;
                }
            }  // namespace encode

            template <int32_t ID>
            struct Message {
                template <typename... Members>
                Message(Members&&... members) {}  //: members(std::forward<Members>(members)...) {}

                // template <int... Is>
                // void encode(std::vector<char>& data, std::index_sequence<Is...>) {
                //     util::unpack((std::get<Is>(members).encode(data), 0)...);
                // }

                void encode(std::vector<char>& data) {
                    // data.insert(data.end(), 2, 0);
                    // size_t start = data.size();

                    // // Call encode on each member
                    // encode(data, std::index_sequence_for<Members...>());

                    // encode::redundant_varint(uint32_t(ID << 3 | 2), 2, std::next(data.begin(), start));
                }
                operator std::vector<char>() {
                    std::vector<char> data;
                    // encode(data);
                    return data;
                }
                // std::tuple<Members...> members;
            };

            template <typename T, int32_t ID>
            struct VarInt {
                VarInt(const T& value) : value(value) {}
                void encode(std::vector<char>& data) {
                    encode::varint(uint32_t(ID << 3 | 0), value, std::back_inserter(data));
                    encode::varint(value, std::back_inserter(data));
                }
                T value;
            };

            template <typename T, int32_t ID>
            struct Fixed {
                Fixed(const T& value) : value(value) {}
                void encode(std::vector<char>& data) {
                    encode::varint(uint32_t(ID << 3 | 1), value, std::back_inserter(data));
                    encode::fixed(value, std::back_inserter(data));
                }
                T value;
            };

            template <int32_t ID>
            struct string {
                string(std::string value) : value(value) {}
                void encode(std::vector<char>& data) {
                    encode::varint(uint32_t(ID << 3 | 2),
                                   std::distance(value.begin(), value.end()),
                                   std::back_inserter(data));
                    encode::length_delimited(value.begin(), value.end(), std::back_inserter(data));
                }
                std::string value;
            };

            template <int ID>
            using uint64 = VarInt<uint64_t, ID>;
            template <int ID>
            using int64 = VarInt<int64_t, ID>;
            template <int ID>
            using uint32 = VarInt<uint32_t, ID>;
            template <int ID>
            using int32 = VarInt<int32_t, ID>;
            template <int ID>
            using fixed64 = Fixed<uint64_t, ID>;

        }  // namespace protobuf
    }  // namespace trace
}  // namespace extension
}  // namespace NUClear

#endif  // NUCLEAR_EXTENSION_TRACE_PROTOBUF_HPP
