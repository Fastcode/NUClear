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

#ifndef NUCLEAR_DSL_OPERATION_TYPE_BIND_HPP
#define NUCLEAR_DSL_OPERATION_TYPE_BIND_HPP

#include "../store/TypeCallbackStore.hpp"

namespace NUClear {

// Forward declarations
namespace message {
    struct ReactionEvent;
    struct ReactionStatistics;
}  // namespace message

namespace dsl {
    namespace operation {

        // Disable emitting stats for triggers on types that would cause a loop
        template <typename T>
        struct EmitStats : std::true_type {};
        template <>
        struct EmitStats<message::ReactionEvent> : std::false_type {};
        template <>
        struct EmitStats<message::ReactionStatistics> : std::false_type {};

        /**
         * Binds a function to execute when a specific type is emitted.
         *
         * A common pattern in NUClear is to execute a function when a particular type is emitted.
         * This utility class is used to simplify executing a function when a type is emitted.
         * To use this utility inherit from this type with the DataType to listen for.
         * If the callback also needs the data emitted you should also extend from CacheGet.
         *
         * @tparam DataType the data type that will be bound to
         */
        template <typename DataType>
        struct TypeBind {

            template <typename DSL>
            static void bind(const std::shared_ptr<threading::Reaction>& reaction) {

                // Set this reaction as no stats emitting
                reaction->emit_stats &= EmitStats<DataType>::value;

                // Our unbinder to remove this reaction
                reaction->unbinders.push_back([](const threading::Reaction& r) {
                    auto& vec = store::TypeCallbackStore<DataType>::get();

                    auto it = std::find_if(
                        std::begin(vec),
                        std::end(vec),
                        [&r](const std::shared_ptr<threading::Reaction>& item) { return item->id == r.id; });

                    // If the item is in the list erase the item
                    if (it != std::end(vec)) {
                        vec.erase(it);
                    }
                });

                // Create our reaction and store it in the TypeCallbackStore
                store::TypeCallbackStore<DataType>::get().push_back(reaction);
            }
        };


    }  // namespace operation
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_OPERATION_TYPE_BIND_HPP
