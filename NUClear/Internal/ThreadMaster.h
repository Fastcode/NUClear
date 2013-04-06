#ifndef NUCLEAR_THREADMASTER_H
#define NUCLEAR_THREADMASTER_H
#include <typeindex>
#include <map>
#include <memory>
#include <vector>
#include <iostream>
#include <iterator>
#include <thread>
#include "TaskScheduler.h"
#include "ThreadWorker.h"

namespace NUClear {
namespace Internal {

    template <typename TParent>
    class ThreadMaster {
        class Implementation {
            public:
                Implementation(ThreadMaster* parent) :
                    parent(parent){
                }
                
                ~Implementation() {
                }

            protected:
                void start() {
                    for(int i = 0; i < numThreads; ++i) {
                        ThreadWorker thread();
                        m_threads.insert(std::pair<std::thread::id, std::thread>(thread.getThreadId(), std::move(thread)));
                    }
                }
            private:
                ThreadMaster* parent;
                std::map<std::thread::id, ThreadWorker> m_threads;

                TaskScheduler scheduler;
                int numThreads = 4;
        };
    public:
        ThreadMaster() :
            threadmaster(this) {
        }
        
        
        Implementation threadmaster;
    };
}
}
#endif
