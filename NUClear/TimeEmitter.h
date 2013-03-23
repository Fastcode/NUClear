#ifndef NUCLEAR_TIMEEMITTER_H
#define NUCLEAR_TIMEEMITTER_H

#include <functional>
#include <thread>
#include <chrono>

namespace NUClear {
    class TimeEmitter {
        public:
            TimeEmitter();
            ~TimeEmitter();
            void add(std::chrono::nanoseconds nanos, std::function<void ()> emit);
        private:
            bool execute;
            std::thread thread;
            void run();
    };
}

#endif