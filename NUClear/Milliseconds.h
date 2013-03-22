#ifndef NUCLEAR_MILLISECONDS_H
#define NUCLEAR_MILLISECONDS_H

#include "TimeEmitter.h"
#include "ReactorController.h"

namespace NUClear {
    template <int millis>
    class Milliseconds {
        private:
            Milliseconds();
            ReactorController& reactorControl;
            static TimeEmitter e;
            class TimeSubscriber {
                TimeSubscriber() {
                    e.add(millis, [](){
                        reactorControl.emit(Milliseconds<millis>());
                    });
                }
            };
            static TimeSubscriber subscribe;
    };
}

#endif