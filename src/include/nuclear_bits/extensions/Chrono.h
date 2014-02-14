/*
 * Copyright (C) 2013 Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
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

#ifndef NUCLEAR_EXTENSIONS_CHRONO_H
#define NUCLEAR_EXTENSIONS_CHRONO_H

#include "nuclear"

namespace NUClear {
    
    /**
     * @brief This contains all of the information needed to setup a new every.
     *
     * @details
     *  This is created when an exists for an every happens, it is then direct emitted to the Chronomaster object.
     *
     * @author Trent Houliston
     */
    struct ChronoConfig {
        std::type_index type;
        std::function<void ()> emitter;
        clock::duration step;
    };
    
    /**
     * @brief Partial specialization to handle the case when a normal every is used.
     *
     * @tparm ticks the number of ticks that should be waited between each every.
     * @tparm period the type that the ticks are measured in.
     */
    template <int ticks, class period>
    struct Reactor::Exists<dsl::Every<ticks, period>> {
        
        /**
         * @brief This is run when an every exists, it emits a configuration to the chrono system.
         *
         * @param context, a pointer to the reactor that is using the exists.
         */
        static void exists(Reactor* context) {
            
            // Direct emit a ChronoConfig type for our desired length of time
            context->emit<Scope::DIRECT>(std::unique_ptr<ChronoConfig>(new ChronoConfig {
                typeid(dsl::Every<ticks, period>),
                [context] { // Function that emits our timepoint when needed
                    context->emit(std::make_unique<DataFor<dsl::Every<ticks, period>, Reactor::time_t>>(std::make_shared<Reactor::time_t>(clock::now())));
                },
                clock::duration(period(ticks))
            }));
        }
    };
    
    /**
     * @brief Partial specialization to handle the case when an every per is used.
     *
     * @tparm ticks the divisor for the slice of time units to be measured in.
     * @tparm period the time unit we are slicing.
     */
    template <int ticks, class period>
    struct Reactor::Exists<dsl::Every<ticks, dsl::Per<period>>> {
        
        /**
         * @brief This is run when an every exists, it emits a configuration to the chrono system.
         *
         * @param context, a pointer to the reactor that is using the exists.
         */
        static void exists(Reactor* context) {
            
            // Direct emit a ChronoConfig type for our desired length of time
            context->emit<Scope::DIRECT>(std::unique_ptr<ChronoConfig>(new ChronoConfig {
                typeid(dsl::Every<ticks, dsl::Per<period>>),
                [context] { // Function that emits our timepoint when needed
                    context->emit(std::make_unique<DataFor<dsl::Every<ticks, dsl::Per<period>>, Reactor::time_t>>(std::make_shared<Reactor::time_t>(clock::now())));
                },
                // Work out our time unit needed to do an every per
                clock::duration(long((double(period::period::den) / (double(ticks) * double(period::period::num))) * (double(clock::period::den) / clock::period::num)))
            }));
        }
    };
    
    /**
     * @brief Set the data source for an Every to trigger on to use.
     *
     * @details
     *  Since we don't actually trigger on the DSL type, we use the data that was stored with the type.
     *
     * @tparm ticks the divisor for the slice of time units to be measured in.
     * @tparm period the time unit we are slicing.
     */
    template <int ticks, class period>
    struct Reactor::TriggerType<dsl::Every<ticks, period>> {
        typedef DataFor<dsl::Every<ticks, period>, Reactor::time_t> type;
    };
    
    /**
     * @brief When getting data, make sure we use the DataFor instead of the normal store.
     *
     * @tparm ticks the divisor for the slice of time units to be measured in.
     * @tparm period the time unit we are slicing.
     */
    template <int ticks, class period>
    struct PowerPlant::CacheMaster::Get<dsl::Every<ticks, period>> {
        static std::shared_ptr<clock::time_point> get(PowerPlant* context) {
            return ValueCache<DataFor<dsl::Every<ticks, period>, Reactor::time_t>>::get()->data;
        }
    };
    
    namespace extensions {
        
        /**
         * @brief The Chronomaster is a Service thread that manages the Every class time emissions.
         *
         * @details
         *  This reactor when installed makes a service thread that waits for the required amount
         *  of time before emitting an Every for each of the requested everys. It will also do
         *  corrections so that the times are periodically correct (if it overshoots it will
         *  compensate).
         */
        class Chrono : public Reactor {
        public:
            /// @brief Construct a new ThreadMaster with our PowerPlant as context
            Chrono(std::unique_ptr<Environment> environment);
            
            /**
             * @brief Adds a new timeperiod to count and send out events for.
             *
             * @param config contains the information required to configure the new
             */
            void add(const ChronoConfig& config);
            
        private:
            /// @brief this class holds the callbacks to emit events, as well as when to emit these events.
            struct Step {
                /// @brief the size our step is measured in (the size our clock uses)
                clock::duration step;
                /// @brief the time at which we need to emit our next event
                clock::time_point next;
                /// @brief the callbacks to emit for this time (e.g. 1000ms and 1second will trigger on the same tick)
                std::vector<std::function<void ()>> callbacks;
            };
            
            /// @brief Our Run method for the task scheduler, starts the Every events emitting
            void run();
            /// @brief Our Kill method for the task scheduler, shuts down the chronomaster and stops emitting events
            void kill();
            
            /// @brief a mutex which is responsible for controlling if the system should continue to run
            std::timed_mutex execute;
            /// @brief a lock which will be unlocked when the system should finish executing
            std::unique_lock<std::timed_mutex> lock;
            /// @brief A vector of steps containing the callbacks to execute, is sorted regularly to maintain the order
            std::vector<Step> steps;
            /// @brief A list of types which have already been loaded (to avoid duplication)
            std::set<std::type_index> loaded;
        };
    }
}

#endif
