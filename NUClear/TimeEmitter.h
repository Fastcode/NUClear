#ifndef NUCLEAR_TIMEEMITTER_H
#define NUCLEAR_TIMEEMITTER_H

#include <functional>
#include <thread>
#include <chrono>
#include <vector>

namespace NUClear {
    class TimeEmitter {
        public:
            TimeEmitter();
            ~TimeEmitter();
            void add(std::chrono::nanoseconds nanos, std::function<void ()> emit);
        private:
            struct Step {
                std::chrono::nanoseconds step;
                std::chrono::nanoseconds next;
                std::vector<std::function<void ()>> callbacks;
            };
        
            std::chrono::nanoseconds start;
            bool execute;
            std::thread thread;
            std::vector<std::unique_ptr<Step>> steps;
            void run();
    };
}

#endif