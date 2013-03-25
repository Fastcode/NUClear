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

            class TimeSubscriber {
                ReactorController& reactorController;
                TimeSubscriber() : reactorController(ReactorControl) {
                    reactorController.addEvery(std::chrono::duration_cast<std::chrono::nanoseconds>(period(ticks)), [this]() {
                        this->reactorControl.emit(new Every<ticks, period>());
                    });
                }
            };
            static TimeSubscriber subscribe;
    };
    
    template <int ticks, class period>
    typename Every<ticks, period>::TimeSubscriber Every<ticks, period>::subscribe;
}

#endif
