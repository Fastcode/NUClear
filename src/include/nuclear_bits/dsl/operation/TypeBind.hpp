/*
 * Copyright (C) 2013-2016 Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
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

#ifndef NUCLEAR_DSL_OPERATION_TYPEBIND_HPP
#define NUCLEAR_DSL_OPERATION_TYPEBIND_HPP

#include "nuclear_bits/dsl/store/TypeCallbackStore.hpp"
#include "nuclear_bits/util/get_identifier.hpp"

namespace NUClear {
namespace dsl {
    namespace operation {

        /**
         * @brief Binds a function to execute when a specific type is emitted
         *
         * @details A common pattern in NUClear is to execute a function when a particular type is emitted.
         *          This utility class is used to simplify executing a function when a type is emitted.
         *          To use this utility inherit from this type with the DataType to listen for.
         *          If the callback also needs the data emitted you should also extend from CacheGet
         *
         * @tparam DataType the data type that will be bound to
         */
        template <typename DataType>
        struct TypeBind {

            template <typename DSL, typename Function>
            static inline threading::ReactionHandle bind(Reactor& reactor,
                                                         const std::string& label,
                                                         Function&& callback) {

                // Our unbinder to remove this reaction
                std::function<void(threading::Reaction&)> unbinder([](threading::Reaction& r) {

                    auto& vec = store::TypeCallbackStore<DataType>::get();

                    auto item = std::find_if(
                        std::begin(vec), std::end(vec), [&r](const std::shared_ptr<threading::Reaction>& item) {
                            return item->id == r.id;
                        });

                    // If the item is in the list erase the item
                    if (item != std::end(vec)) {
                        vec.erase(item);
                    }
                });

                // Get our identifier string
                std::vector<std::string> identifier =
                    util::get_identifier<typename DSL::DSL, Function>(label, reactor.reactor_name);

                auto reaction = std::make_shared<threading::Reaction>(
                    reactor, std::move(identifier), std::forward<Function>(callback), std::move(unbinder));
                threading::ReactionHandle handle(reaction);

                // Create our reaction and store it in the TypeCallbackStore
                store::TypeCallbackStore<DataType>::get().push_back(std::move(reaction));

                // Return our handle
                return handle;
            }
        };

    }  // namespace operation
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_OPERATION_TYPEBIND_HPP
