#ifndef NUCLEAR_MILLISECONDS_H
#define NUCLEAR_MILLISECONDS_H

#include <chrono>
#include "TimeEmitter.h"
#include "ReactorController.h"

namespace NUClear {
    template <int ticks, class period = std::chrono::milliseconds>
    class Every {
        private:
            Every() {
            }

            static TimeEmitter e;

            class TimeSubscriber {
                ReactorController& reactorController;
                TimeSubscriber() : reactorController(ReactorControl) {
                    e.add(std::chrono::duration_cast<std::chrono::nanoseconds>(period(ticks)), [this]() {
                        this->reactorControl.emit(new Every<ticks, period>());
                    });
                }
            };
            static TimeSubscriber subscribe;
    };

    // Let there be statics!
    template <int ticks, class period>
    TimeEmitter Every<ticks, period>::e;
    
    template <int ticks, class period>
    typename Every<ticks, period>::TimeSubscriber Every<ticks, period>::subscribe;
}

#endif
