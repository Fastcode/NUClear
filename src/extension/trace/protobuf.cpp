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

#include "protobuf.hpp"

#include <cstdint>
#include <iterator>
#include <type_traits>
#include <vector>

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
                    *out++ = v & 0x7F;
                    return out;
                }

                template <typename T, typename OutputIterator, std::enable_if_t<std::is_integral<T>::value, int> = 0>
                OutputIterator fixed(const T& value, OutputIterator out) {
                    // Cast the value down to its unsigned type and encode it as fixed
                    const std::make_unsigned_t<T> v(value);
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

            namespace field {
                template <typename T, typename OutputIterator>
                OutputIterator varint(const uint32_t& id, const T& value, OutputIterator out) {
                    out = encode::varint(uint32_t(id << 3 | 0), out);
                    return encode::varint(value, out);
                }

                template <typename T, typename OutputIterator>
                OutputIterator fixed(const uint32_t& id, const T& value, OutputIterator out) {
                    out = encode::varint(uint32_t(id << 3 | 1), out);
                    return encode::fixed(value, out);
                }

                template <typename Iterator, typename OutputIterator>
                OutputIterator length_delimited(const uint32_t& id, Iterator begin, Iterator end, OutputIterator out) {
                    out = encode::varint(uint32_t(id << 3 | 2), out);
                    return encode::length_delimited(begin, end, out);
                }

            }  // namespace field

            void uint64(const uint32_t& id, const uint64_t& value, std::vector<char>& data) {
                field::varint(id, value, std::back_inserter(data));
            }

            void int64(const uint32_t& id, const int64_t& value, std::vector<char>& data) {
                field::varint(id, value, std::back_inserter(data));
            }

            void fixed64(const uint32_t& id, const uint64_t& value, std::vector<char>& data) {
                field::fixed(id, value, std::back_inserter(data));
            }

            void uint32(const uint32_t& id, const uint32_t& value, std::vector<char>& data) {
                field::varint(id, value, std::back_inserter(data));
            }

            void int32(const uint32_t& id, const int32_t& value, std::vector<char>& data) {
                field::varint(id, value, std::back_inserter(data));
            }

            void string(const uint32_t& id, const std::string& value, std::vector<char>& data) {
                field::length_delimited(id, value.begin(), value.end(), std::back_inserter(data));
            }

            SubMessage::SubMessage(const uint32_t& id, std::vector<char>& data, size_t varint_bytes)
                : data(data), varint_bytes(varint_bytes) {
                encode::varint(uint32_t(id << 3 | 2), std::back_inserter(data));  // Type and id
                // C'mon clang-tidy I literally just changed the vector on the line above
                // NOLINTNEXTLINE(cppcoreguidelines-prefer-member-initializer)
                start = data.size();  // Store the current position so we can write the length later
                data.insert(data.end(), varint_bytes, 0);  // Reserve space for the length
            }
            SubMessage::~SubMessage() {
                // Update the length of the SubMessage
                auto size = uint32_t(data.size() - start - varint_bytes);
                auto it   = std::next(data.begin(), ptrdiff_t(start));
                encode::redundant_varint(size, varint_bytes, it);
            }

        }  // namespace protobuf
    }  // namespace trace
}  // namespace extension
}  // namespace NUClear
