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
#include <string>
#include <type_traits>
#include <vector>

namespace NUClear {
namespace extension {
    namespace trace {
        namespace protobuf {

            /**
             * An RAII type which will encode the size of a sub-message after it has been inserted.
             *
             * When constructing this type it will take note of where in the data vector it is and store it.
             * Then when it is destructed it will write back the size of the sub-message to the correct location.
             * It will ensure that enough storage space is available for the size by encoding it using a redundant
             * varint.
             *
             * This redundant varint will always take up the same number of bytes so the vector does not need to be
             * resized.
             *
             * By default it reserves 2 bytes for the varint which will allow a maximum size of 16384 (2^14) bytes.
             * If the sub-message is larger than this then the vector will be resized and the varint will be updated.
             */
            class SubMessage {
            public:
                /**
                 * Construct a new Sub Message RAII object
                 *
                 * @param id The protobuf id of the sub-message field
                 * @param data The data vector to write the sub-message to
                 * @param varint_bytes The number of bytes to reserve for the varint encoding the size of the
                 * sub-message
                 */
                SubMessage(const uint32_t& id, std::vector<char>& data, size_t varint_bytes = 2);
                ~SubMessage();
                SubMessage(const SubMessage&)            = delete;
                SubMessage(SubMessage&&)                 = delete;
                SubMessage& operator=(const SubMessage&) = delete;
                SubMessage& operator=(SubMessage&&)      = delete;

            private:
                /// The data vector to write the sub-message to
                std::vector<char>& data;
                /// The number of bytes to reserve for the varint encoding the size of the sub-message
                size_t varint_bytes;
                /// The position in the data vector where the size of the sub-message is stored
                size_t start;
            };

            /**
             * Encodes a uint64 value into the data vector.
             *
             * @param id    The protobuf id of the field
             * @param value The uint64 value to encode
             * @param data  The data vector to write the encoded value to
             */
            void uint64(const uint32_t& id, const uint64_t& value, std::vector<char>& data);
            /**
             * Encodes a int64 value into the data vector.
             *
             * @param id    The protobuf id of the field
             * @param value The int64 value to encode
             * @param data  The data vector to write the encoded value to
             */
            void int64(const uint32_t& id, const int64_t& value, std::vector<char>& data);
            /**
             * Encodes a fixed64 value into the data vector.
             *
             * @param id    The protobuf id of the field
             * @param value The fixed64 value to encode
             * @param data  The data vector to write the encoded value to
             */
            void fixed64(const uint32_t& id, const uint64_t& value, std::vector<char>& data);

            /**
             * Encodes a uint32 value into the data vector.
             *
             * @param id    The protobuf id of the field
             * @param value The uint32 value to encode
             * @param data  The data vector to write the encoded value to
             */
            void uint32(const uint32_t& id, const uint32_t& value, std::vector<char>& data);

            /**
             * Encodes a int32 value into the data vector.
             *
             * @param id    The protobuf id of the field
             * @param value The int32 value to encode
             * @param data  The data vector to write the encoded value to
             */
            void int32(const uint32_t& id, const int32_t& value, std::vector<char>& data);

            /**
             * Encodes a string value into the data vector.
             *
             * @param id    The protobuf id of the field
             * @param value The string value to encode
             * @param data  The data vector to write the encoded value to
             */
            void string(const uint32_t& id, const std::string& value, std::vector<char>& data);

        }  // namespace protobuf
    }  // namespace trace
}  // namespace extension
}  // namespace NUClear

#endif  // NUCLEAR_EXTENSION_TRACE_PROTOBUF_HPP
