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

#ifndef NUCLEAR_EXTENSION_TRACE_STRING_INTERNER_HPP
#define NUCLEAR_EXTENSION_TRACE_STRING_INTERNER_HPP

#include <cstdint>
#include <functional>
#include <map>
#include <utility>
#include <vector>

#include "protobuf.hpp"

namespace NUClear {
namespace extension {
    namespace trace {

        /**
         * This class is used to intern strings into a protobuf trace file.
         *
         * It holds a map of keys to their interned ids and writes the data to the trace file when a new key is added.
         *
         * @tparam Key The type of the key to intern
         * @tparam ID  The protobuf id of the interned data
         */
        template <typename Key, int32_t ID>
        struct StringInterner {

            /**
             * Construct a new String Interner object
             *
             * @param trusted_packet_sequence_id The trusted packet sequence id to use for the trace file
             * @param make                       The function which creates the string from the key
             * @param write                      The function which writes the data to the trace file
             */
            StringInterner(const uint32_t& trusted_packet_sequence_id,
                           std::function<std::string(const Key&)> make,
                           std::function<void(const std::vector<char>&)> write)
                : trusted_packet_sequence_id(trusted_packet_sequence_id)
                , make(std::move(make))
                , write(std::move(write)) {}

            /**
             * Get the interned id for the key.
             *
             * @param key The key to get the interned id for
             *
             * @return The interned id for the key
             */
            template <typename U = Key>
            uint64_t operator[](const U& key) {
                auto it = interned.find(key);
                if (it != interned.end()) {
                    return it->second;
                }

                const uint64_t iid = interned.emplace(key, interned.size() + 1).first->second;
                std::vector<char> data;
                {
                    const protobuf::SubMessage packet(1, data);              // packet:1
                    protobuf::uint32(10, trusted_packet_sequence_id, data);  // trusted_packet_sequence_id:10:uint32
                    {
                        const protobuf::SubMessage interned_data(12, data);  // interned_data:12
                        {
                            const protobuf::SubMessage interned_type(ID, data);  // interned_type:{ID}
                            protobuf::uint64(1, iid, data);                      // iid:1:uint64
                            protobuf::string(2, make(key), data);                // name:2:string
                        }
                    }
                }
                write(data);

                return iid;
            }

        private:
            /// The trusted packet sequence id to use for the trace file
            const uint32_t trusted_packet_sequence_id;
            /// The keys mapped to their interned ids
            std::map<Key, uint64_t, std::less<>> interned;
            /// The function which creates the string from the key
            std::function<std::string(const Key&)> make;
            /// The function which writes the data to the trace file
            std::function<void(const std::vector<char>&)> write;
        };

    }  // namespace trace
}  // namespace extension
}  // namespace NUClear

#endif  // NUCLEAR_EXTENSION_TRACE_STRING_INTERNER_HPP
