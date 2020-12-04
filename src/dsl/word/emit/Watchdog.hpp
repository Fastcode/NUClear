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

#ifndef NUCLEAR_DSL_WORD_EMIT_WATCHDOG_HPP
#define NUCLEAR_DSL_WORD_EMIT_WATCHDOG_HPP

#include "../../../PowerPlant.hpp"
#include "../../../util/TypeMap.hpp"
#include "../../../util/demangle.hpp"

namespace NUClear {
namespace dsl {
    namespace word {
        namespace emit {

            /**
             * @brief
             *  When emitting data under this scope, the service time for the watchdog is updated
             *
             * @details
             *  @code emit<Scope::WATCHDOG>(data, dataType); @endcode
             *
             * @param data
             *  the data to emit
             * @tparam DataType
             *  the datatype of the object to emit
             */
            template <typename DataType>
            struct Watchdog {

                static void emit(PowerPlant& powerplant,
                                 std::shared_ptr<DataType> data = std::make_shared<DataType>()) {

                    // Find our data store
                    using WatchdogStore = util::TypeMap<DataType, void, std::map<DataType, NUClear::clock::time_point>>;

                    // Make sure the store has already been created and that our sub type has been entered into the
                    // store
                    if (WatchdogStore::get() == nullptr || WatchdogStore::get()->count(*data) == 0) {
                        throw std::runtime_error("Store for <" + util::demangle(typeid(DataType).name())
                                                 + "> has not been created yet or no watchdog has been set up");
                    }

                    // Update our service time
                    WatchdogStore::get()->operator[](*data) = NUClear::clock::now();
                }
            };

        }  // namespace emit
    }      // namespace word
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_WORD_EMIT_WATCHDOG_HPP
