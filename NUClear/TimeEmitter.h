#ifndef NUCLEAR_TIMEEMITTER_H
#define NUCLEAR_TIMEEMITTER_H

#include <functional>
#include <thread>

namespace NUClear {
    class TimeEmitter {
        public:
            TimeEmitter();
            ~TimeEmitter();
            void add(int millis, std::function<void ()> emit);
        private:
            std::thread thread;
            void run();
    };
}

#endif