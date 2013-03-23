#include "TimeEmitter.h"

namespace NUClear {
    
    TimeEmitter::TimeEmitter()
        : start(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch())),
          execute(true),
          thread(std::bind(&TimeEmitter::run, this)) {
    }
    
    TimeEmitter::~TimeEmitter() {
        execute = false;
        thread.detach();
    }
    
    void TimeEmitter::add(std::chrono::nanoseconds step, std::function<void ()> emit) {
        
        // TODO combine the callbacks for each step object somehow (probably a unique ptr along with a time in a map)
        Step s;
        s.step = step;
        s.next = start;
        s.callbacks.push_back(emit);
    }
    
    void TimeEmitter::run() {
        
        while(execute) {
            std::chrono::nanoseconds now = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch());
            for(auto it = std::begin(steps); it != std::end(steps); ++it) {
                now = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch());
                if((it->next - now).count() <= 0) {
                    for(auto callback = std::begin(it->callbacks); callback != std::end(it->callbacks); ++callback) {
                        (*callback)();
                    }
                    it->next += it->step;
                }
            }
            
            std::sort(std::begin(steps), std::end(steps), [](Step a, Step b) {
                return a.next < b.next;
            });
            
            std::this_thread::sleep_for(steps.front().next - now);
        }
    }
}
