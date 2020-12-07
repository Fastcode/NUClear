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

#ifndef NUCLEAR_DSL_WORD_WATCHDOG_HPP
#define NUCLEAR_DSL_WORD_WATCHDOG_HPP

#include "../../util/demangle.hpp"
#include "../operation/Unbind.hpp"
#include "../store/DataStore.hpp"
#include "emit/Direct.hpp"

namespace NUClear {
namespace dsl {
    namespace word {

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
        struct WatchdogDataStore {
            using MapType       = std::remove_cv_t<RuntimeType>;
            using WatchdogStore = util::TypeMap<WatchdogGroup, MapType, std::map<MapType, NUClear::clock::time_point>>;

            /**
             * @brief Ensures the data store is initialised correctly
             *
             * @param data The runtime argument for the current watchdog in the WatchdogGroup/RuntimeType group
             */
            static void init(const RuntimeType& data) {
                if (WatchdogStore::get() == nullptr) {
                    WatchdogStore::set(std::make_shared<std::map<MapType, NUClear::clock::time_point>>());
                }
                if (WatchdogStore::get()->count(data) == 0) {
                    WatchdogStore::get()->insert({data, NUClear::clock::now()});
                }
            }

            /**
             * @brief Gets the current service time for the WatchdogGroup/RuntimeType/data watchdog
             *
             * @param data The runtime argument for the current watchdog in the WatchdogGroup/RuntimeType group
             */
            static const NUClear::clock::time_point& get(const RuntimeType& data) {
                if (WatchdogStore::get() == nullptr || WatchdogStore::get()->count(data) == 0) {
                    throw std::runtime_error("Store for <" + util::demangle(typeid(WatchdogGroup).name()) + ", "
                                             + util::demangle(typeid(MapType).name())
                                             + "> is trying to field a service call for an unknown data type");
                }
                return WatchdogStore::get()->at(data);
            }

            /**
             * @brief Cleans up any allocated storage for the WatchdogGroup/RuntimeType/data watchdog
             *
             * @param data The runtime argument for the current watchdog in the WatchdogGroup/RuntimeType group
             */
            static void unbind(const RuntimeType& data) {
                if (WatchdogStore::get() != nullptr) { WatchdogStore::get()->erase(data); }
            }
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
        struct WatchdogDataStore<WatchdogGroup, void> {
            using WatchdogStore = util::TypeMap<WatchdogGroup, void, NUClear::clock::time_point>;

            /**
             * @brief Ensures the data store is initialised correctly
             */
            static void init() {
                if (WatchdogStore::get() == nullptr) {
                    WatchdogStore::set(std::make_shared<NUClear::clock::time_point>(NUClear::clock::now()));
                }
            }

            /**
             * @brief Gets the current service time for the WatchdogGroup watchdog
             */
            static const NUClear::clock::time_point& get() {
                if (WatchdogStore::get() == nullptr) {
                    throw std::runtime_error("Store for <" + util::demangle(typeid(WatchdogGroup).name())
                                             + "> is trying to field a service call for an unknown data type");
                }
                return *WatchdogStore::get();
            }

            /**
             * @brief Cleans up any allocated storage for the WatchdogGroup watchdog
             */
            static void unbind() {
                if (WatchdogStore::get() != nullptr) { WatchdogStore::get().reset(); }
            }
        };

        /**
         * @brief
         *  This can be used to monitor task(s); if the monitored task(s) have not occurred within a desired timeframe,
         *  the watchdog can be serviced to trigger a specified reaction.
         *
         * @details
         *  @code on<Watchdog<WatchdogGroup, ticks, period>>() @endcode
         *  This is a useful tool for anything in the system which might stall, and needs to be kick-started.
         *
         *  The watchdog can monitor a single task, or group of tasks, over a period of time. If no activity is
         *  detected after the specified timeframe, the watchdog will be serviced.  When the watchdog is serviced, the
         *  timer resets.
         *
         * @par Single Reaction
         *  @code on<Watchdog<SampleReaction, 10, std::chrono::milliseconds>>() @endcode
         *  In the example above, a SampleReaction will be monitored. If the reactions does not occur within 10
         *  milliseconds, the watchdog will be serviced.
         *
         * @par Group of Reactions
         *  @code on<Watchdog<SampleReactor, 10, std::chrono::milliseconds>>() @endcode
         *  In the example above, all reactions from the SampleReactor will be monitored. If a task associated with the
         *  SampleReactor has not occurred for 10 milliseconds, the watchdog will be serviced.
         *
         * @par Multiple watchdogs in a single reactor
         *  @code on<Watchdog<SampleReactor, 10, std::chrono::milliseconds>>(data) @endcode
         *  In the example above, all reactions from the SampleReactor will be monitored, but a different watchdog will
         *  be created for each unique instance of data. If the task associated with an instance of data in
         *  the SampleReactor group has not occurred for 10 milliseconds, the watchdog for that instance of data will
         *  be serviced.
         *
         * @par Service the Watcdog
         *  @code  emit<Scope::WATCHDOG>(ServiceWatchdog<SampleReactor>()) @endcode
         *  The watchdog will need to be serviced by a watchdog service emission. The emission must use the same
         *  template type as the watchdog. Each time this emission occurs, the watchdog timer will be reset.
         *
         *  @code  emit<Scope::WATCHDOG>(ServiceWatchdog<SampleReactor>(data)) @endcode
         *  The watchdog will need to be serviced by a watchdog service emission. The emission must use the same
         *  template type and runtime argument as the watchdog. Each time this emission occurs, the watchdog timer
         *  for the specified runtime argument will be reset.
         *
         * @attention
         *  The period which is used to measure the ticks must be greater than or equal to clock::duration or the
         *  program will not compile.
         *
         * @par Implements
         *  Bind
         *
         * @tparam WatchdogGroup
         *  the type/group of tasks the watchdog will track.   This needs to be a declared type within the system (be it
         *  a reactor, reaction, or other type).
         * @tparam ticks
         *  the number of ticks of a particular type to wait
         * @tparam period
         *  a type of duration (e.g. std::chrono::seconds) to measure the ticks in.  This will default to clock
         *  duration, but can accept any of the defined std::chrono durations (nanoseconds, microseconds, milliseconds,
         *  seconds, minutes, hours).  Note that you can also define your own unit:  See
         *  http://en.cppreference.com/w/cpp/chrono/duration
         */
        template <typename WatchdogGroup, int ticks, class period>
        struct Watchdog {

            /**
             * @brief Binder for Watchdog reactions with a runtime argument
             *
             * @tparam DSL
             *
             * @tparam RuntimeType
             *  the type of the runtime argument. const/volatile specifiers are stripped from this type
             * @param reaction the reaction object that we are binding
             * @param data the runtime argument for the current watchdog in the WatchdogGroup/RuntimeType group
             */
            template <typename DSL, typename RuntimeType>
            static inline void bind(const std::shared_ptr<threading::Reaction>& reaction, const RuntimeType& data) {

                // Make sure the store is initialised
                WatchdogDataStore<WatchdogGroup, RuntimeType>::init(data);

                // Create our unbinder
                reaction->unbinders.push_back([data](const threading::Reaction& r) {
                    // Remove the active service time from the data store
                    WatchdogDataStore<WatchdogGroup, RuntimeType>::unbind(data);
                    r.reactor.emit<emit::Direct>(std::make_unique<operation::Unbind<operation::ChronoTask>>(r.id));
                });

                // Send our configuration out
                reaction->reactor.emit<emit::Direct>(std::make_unique<operation::ChronoTask>(
                    [reaction, data](NUClear::clock::time_point& time) {
                        return Watchdog::chrono_task(
                            reaction, WatchdogDataStore<WatchdogGroup, RuntimeType>::get(data), time);
                    },
                    NUClear::clock::now() + period(ticks),
                    reaction->id));
            }

            /**
             * @brief Binder for Watchdog reactions with no runtime argument
             */
            template <typename DSL>
            static inline void bind(const std::shared_ptr<threading::Reaction>& reaction) {

                // Make sure the store is initialised
                WatchdogDataStore<WatchdogGroup>::init();

                // Create our unbinder
                reaction->unbinders.push_back([](const threading::Reaction& r) {
                    // Remove the active service time from the data store
                    WatchdogDataStore<WatchdogGroup>::unbind();
                    r.reactor.emit<emit::Direct>(std::make_unique<operation::Unbind<operation::ChronoTask>>(r.id));
                });

                // Send our configuration out
                reaction->reactor.emit<emit::Direct>(std::make_unique<operation::ChronoTask>(
                    [reaction](NUClear::clock::time_point& time) {
                        return Watchdog::chrono_task(reaction, WatchdogDataStore<WatchdogGroup>::get(), time);
                    },
                    NUClear::clock::now() + period(ticks),
                    reaction->id));
            }

        private:
            /**
             * @brief updates the service time for the current reaction
             *
             * @param reaction the reaction we are servicing
             * @param service_time the last service time of the watchdog
             * @param time the time when this watchdog should be checked next
             * @return true the chrono task should run again
             * @return false the chrono task should not run again
             */
            static bool chrono_task(const std::shared_ptr<threading::Reaction>& reaction,
                                    const NUClear::clock::time_point& service_time,
                                    NUClear::clock::time_point& time) {

                // Check if our watchdog has timed out
                if (NUClear::clock::now() > (service_time + period(ticks))) {
                    try {
                        // Submit the reaction to the thread pool
                        auto task = reaction->get_task();
                        if (task) { reaction->reactor.powerplant.submit(std::move(task)); }
                    }
                    catch (...) {
                    }

                    // Now automatically service the watchdog
                    time = NUClear::clock::now() + period(ticks);
                }
                // Change our wait time to our new watchdog time
                else {
                    time = service_time + period(ticks);
                }

                // We renew!
                return true;
            }
        };

    }  // namespace word
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_WORD_WATCHDOG_HPP
