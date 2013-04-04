#ifndef NUCLEAR_CHRONOMETER_H
#define NUCLEAR_CHRONOMETER_H
#include <set>
#include <typeindex>
#include <functional>
#include <vector>
#include <algorithm>
#include <thread>

namespace NUClear {
    /**
     * @brief Empty class which specifies a period at which to have an event fired.
     * @tparam ticks the number of ticks between events
     * @tparam period the timescale which ticks is measured in, defaults to milliseconds
     */
    template <int ticks, class period = std::chrono::milliseconds>
    class Every {};
    
    /**
     * @brief Class which is responsible for emitting the Every events to the system at the proper interval
     * @details
     *  This class will emit an every event of the correct type at regular intervals. Due to the way that thread
     *  sleeping works, it may not happen at the exact instant required, however the lag time will be taken into account
     *  and compensated for so the intervals will be regular.
     * @author Trent Houliston
     * @version 1.0
     * @date 3-Apr-2013
     */
    template <typename TReactorController>
    class Chronometer {
        public:
            Chronometer();
            ~Chronometer();
        
            /**
             * @brief Adds a new interval and callback to wait in the system
             * @tparam ticks   the number of ticks to wait
             * @tparam period  a class of type std::chrono::duration to classify the ticks with
             *
             * @param emit     a function which will emit the object of the correct type into the reactor controller
             */
            template <int ticks, class period>
            void add() {
                
                // Check if we have not already loaded this type in
                if(loaded.find(typeid(Every<ticks, period>)) == std::end(loaded)) {
                    
                    // Flag this type as loaded
                    loaded.insert(typeid(Every<ticks, period>));
                    
                    std::function<void ()> emit = [this](){
                        static_cast<TReactorController*>(this)->emit(new Every<ticks, period>());
                    };
                    
                    // Get the number of nanoseconds this tick is
                    std::chrono::nanoseconds step(std::chrono::duration_cast<std::chrono::nanoseconds>(period(ticks)));
                    
                    // See if we already have one with this period
                    auto item = std::find_if(std::begin(steps), std::end(steps), [step](std::unique_ptr<Step>& find) {
                        return find->step == step;
                    });
                    
                    // If we don't then create a new one with our initial data
                    if(item == std::end(steps)) {
                        std::unique_ptr<Step> s(new Step());
                        s->step = step;
                        s->callbacks.push_back(emit);
                        steps.push_back(std::move(s));
                    }
                    
                    // Otherwise just add the callback to the existing element
                    else {
                        (*item)->callbacks.push_back(emit);
                    }
                }
            }
        
            /**
             * @brief Runs the emission system. Sleeps the thread until the next emission and then emits all events
             *        required before sleeping again. Should be run in a seperate thread.
             */
            void run();
        private:
            /// @brief this class holds the callbacks to emit events, as well as when to emit these events.
            struct Step {
                std::chrono::nanoseconds step;
                std::chrono::nanoseconds next;
                std::vector<std::function<void ()>> callbacks;
            };
        
            /// @brief If the system should continue to execute or if it should stop
            bool execute;
        
            /// @brief A vector of steps containing the callbacks to execute, is sorted regularly to maintain the order
            std::vector<std::unique_ptr<Step>> steps;
        
            /// @brief A list of types which have already been loaded (to avoid duplication)
            std::set<std::type_index> loaded;
    };
}

#endif
