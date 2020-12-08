/*
 * Copyright (C) Alex Biddulph <Alexander.Biddulph@uon.edu.au>
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
             *  Handles the data store for the case when runtime arguments specified
             *  @code on<Watchdog<>>(data) @endcode
             *  @code emit<Scope::WATCHDOG>(data) @endcode
             *
             * @tparam WatchdogGroup
             *  the type/group of tasks the watchdog will track. This needs to be a declared type within the system
             * (be it a reactor, reaction, or other type).
             * @tparam RuntimeType
             *  the type of the runtime argument. const/volatile specifiers are stripped from this type
             */
            template <typename WatchdogGroup, typename RuntimeType = void>
            struct WatchdogServicer {
                using MapType = std::remove_cv_t<RuntimeType>;
                using WatchdogStore =
                    util::TypeMap<WatchdogGroup, MapType, std::map<MapType, NUClear::clock::time_point>>;

                WatchdogServicer() : when(NUClear::clock::now()), data() {}
                WatchdogServicer(const RuntimeType& data) : when(NUClear::clock::now()), data(data) {}

                /**
                 * @brief *  Services the watchdog
                 *
                 * @details
                 *  The watchdog timer that is specified by the WatchdogGroup/RuntimeType/data
                 *  combination will have its service time updated to whatever is stored in when
                 */
                void service() {
                    if (WatchdogStore::get() == nullptr || WatchdogStore::get()->count(data) == 0) {
                        throw std::runtime_error("Store for <" + util::demangle(typeid(WatchdogGroup).name()) + ", "
                                                 + util::demangle(typeid(RuntimeType).name())
                                                 + "> has not been created yet or no watchdog has been set up");
                    }
                    WatchdogStore::get()->at(data) = when;
                }

            private:
                NUClear::clock::time_point when;
                RuntimeType data;
            };

            /**
             * @brief
             *  Handles the data store for the case when no runtime arguments are specified
             *  @code on<Watchdog<>>() @endcode
             *  @code emit<Scope::WATCHDOG>() @endcode
             *
             * @tparam WatchdogGroup
             *  the type/group of tasks the watchdog will track. This needs to be a declared type within the system
             * (be it a reactor, reaction, or other type).
             */
            template <typename WatchdogGroup>
            struct WatchdogServicer<WatchdogGroup, void> {
                using WatchdogStore = util::TypeMap<WatchdogGroup, void, NUClear::clock::time_point>;

                WatchdogServicer() : when(NUClear::clock::now()) {}

                /**
                 * @brief *  Services the watchdog
                 *
                 * @details
                 *  The watchdog timer for WatchdogGroup will have its service time updated to whatever is stored in
                 * when
                 */
                void service() {
                    if (WatchdogStore::get() == nullptr) {
                        throw std::runtime_error("Store for <" + util::demangle(typeid(WatchdogGroup).name())
                                                 + "> has not been created yet or no watchdog has been set up");
                    }
                    WatchdogStore::set(std::make_shared<NUClear::clock::time_point>(when));
                }

            private:
                NUClear::clock::time_point when;
            };

            /**
             * @brief
             *  Convenience function to instantiate a WatchdogServicer for a watchdog with a runtime argument
             *
             * @tparam WatchdogGroup
             *  the type/group of tasks the watchdog will track. This needs to be a declared type within the system
             * (be it a reactor, reaction, or other type).
             * @tparam RuntimeType
             *  the type of the runtime argument. const/volatile specifiers are stripped from this type
             * @param data The runtime argument that was passed to
             * @code on<Watchdog<>>(data) @endcode
             * @return WatchdogServicer<WatchdogGroup, RuntimeType>
             */
            template <typename WatchdogGroup, typename RuntimeType>
            WatchdogServicer<WatchdogGroup, RuntimeType> ServiceWatchdog(RuntimeType&& data) {
                return WatchdogServicer<WatchdogGroup, RuntimeType>(std::forward<RuntimeType>(data));
            }

            /**
             * @brief Convenience function to instantiate a WatchdogServicer for a watchdog with no runtime argument
             *
             * @tparam WatchdogGroup
             *  the type/group of tasks the watchdog will track. This needs to be a declared type within the system
             * (be it a reactor, reaction, or other type).
             * @return WatchdogServicer<WatchdogGroup, void>
             */
            template <typename WatchdogGroup>
            WatchdogServicer<WatchdogGroup, void> ServiceWatchdog() {
                return WatchdogServicer<WatchdogGroup, void>();
            }

            /**
             * @brief When emitting data under this scope, the service time for the watchdog is updated
             *
             * @details
             *  @code emit<Scope::WATCHDOG>(ServiceWatchdog<WatchdogGroup>(data)); @endcode
             * or
             *  @code emit<Scope::WATCHDOG>(ServiceWatchdog<WatchdogGroup>()); @endcode
             *
             * The RuntimeType template parameter need not be specified for ServiceWatchdog as it will be inferred from
             * the data argument, if it is specified
             *
             * @tparam the datatype of the object to emit
             */
            template <typename>
            struct Watchdog {

                template <typename WatchdogGroup, typename RuntimeType>
                static void emit(PowerPlant&, WatchdogServicer<WatchdogGroup, RuntimeType>& servicer) {
                    // Update our service time
                    servicer.service();
                }
            };

        }  // namespace emit
    }      // namespace word
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_WORD_EMIT_WATCHDOG_HPP
