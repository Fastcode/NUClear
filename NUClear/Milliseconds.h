#ifndef NUCLEAR_MILLISECONDS_H
#define NUCLEAR_MILLISECONDS_H

#include "TimeEmitter.h"
#include "ReactorController.h"

namespace NUClear {
    template <int milliseconds>
    class Milliseconds {
        private:
            Milliseconds() : 
                reactorControl(ReactorControl) {

            }

            ReactorController& reactorControl;

            static TimeEmitter e;

            class TimeSubscriber {
                TimeSubscriber() {
                    e.add(milliseconds, [this]() {
                        this.reactorControl.emit(Milliseconds<milliseconds>());
                    });
                }
            };
            static TimeSubscriber subscribe;
    };

    // Let there be statics!
    template <int milliseconds> 
    TimeEmitter Milliseconds<milliseconds>::e;

    template <int milliseconds>
    typename Milliseconds<milliseconds>::TimeSubscriber Milliseconds<milliseconds>::subscribe;
}

#endif
