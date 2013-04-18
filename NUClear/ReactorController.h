/**
 * Copyright (C) 2013 Jake Woods <jake.f.woods@gmail.com>, Trent Houliston <trent@houliston.me>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef NUCLEAR_REACTORCONTROLLER_H
#define NUCLEAR_REACTORCONTROLLER_H

#include <memory>
#include <set>
#include <thread>
#include <vector>
#include <typeindex>
#include <mutex>
#include <map>
#include "Internal/ThreadWorker.h"
#include "Internal/TaskScheduler.h"
#include "Internal/CommandTypes/CommandTypes.h"
#include "Internal/Magic/CompiledMap.h"

namespace NUClear {
    
    class Reactor;
    class ReactorController {
        private:
            friend class Reactor;

            class BaseMaster {
                public:
                    BaseMaster(ReactorController* parent); 
                protected:
                    ReactorController* m_parent;
            };
        
            class ThreadMaster : public BaseMaster {
                public:
                    
                    ThreadMaster(ReactorController* parent);
                    
                    void start();
                    void shutdown();
                    void submit(std::unique_ptr<Internal::Reaction::Task>&& task);
                    void internalTask(Internal::ThreadWorker::InternalTask task);
                private:
                    std::map<std::thread::id, std::unique_ptr<Internal::ThreadWorker>> m_threads;
                    std::vector<Internal::ThreadWorker::InternalTask> m_internalTasks;
                    Internal::TaskScheduler m_scheduler;
                    int numThreads = 4;
            };

            class ChronoMaster : public BaseMaster {
                public:
                    ChronoMaster(ReactorController* parent);
                    ~ChronoMaster();

                    template <int ticks, class period>
                    void add();
                private:
                    /// @brief this class holds the callbacks to emit events, as well as when to emit these events.
                    struct Step {
                        std::chrono::steady_clock::duration step;
                        std::chrono::time_point<std::chrono::steady_clock> next;
                        std::vector<std::function<void (std::chrono::time_point<std::chrono::steady_clock>)>> callbacks;
                    };
                    
                    void run();
                    void kill();
                
                    /// @brief a mutex which is responsible for controlling if the system should continue to run
                    std::timed_mutex m_execute;
                    /// @brief a lock which will be unlocked when the system should finish executing
                    std::unique_lock<std::timed_mutex> m_lock;
                    /// @brief A vector of steps containing the callbacks to execute, is sorted regularly to maintain the order
                    std::vector<std::unique_ptr<Step>> m_steps;
                    /// @brief A list of types which have already been loaded (to avoid duplication)
                    std::set<std::type_index> m_loaded;
            };
        
            class CacheMaster : public BaseMaster {
                private:
                    template <typename TData>
                    struct ValueCache {
                        typedef Internal::Magic::CompiledMap<CacheMaster, TData, TData, Internal::Magic::QUEUE> cache;
                    };
                public:
                    CacheMaster(ReactorController* parent);
                    ~CacheMaster();
                
                    template <typename TData>
                    void cache(TData* cache);
                
                    template <typename TData>
                    std::shared_ptr<TData> getData(TData*);
                    
                    template <int num, typename TData>
                    std::shared_ptr<std::vector<std::shared_ptr<const TData>>> getData(Internal::CommandTypes::Last<num, TData>*);
                    
                    template <int ticks, class period>
                    std::shared_ptr<std::chrono::time_point<std::chrono::steady_clock>> getData(Internal::CommandTypes::Every<ticks, period>*);
                
                    template <typename TData>
                    auto get() -> decltype(std::declval<CacheMaster>().getData(std::declval<TData*>())) {
                        return getData(reinterpret_cast<TData*>(0));
                    }
                
                    template <int num, typename TData>
                    void ensureCache();
            };

            class ReactorMaster : public BaseMaster {
                public:
                    ReactorMaster(ReactorController* parent);

                    template <typename TTrigger>
                    void emit(TTrigger* data);
                    
                    template <typename TReactor>
                    void install();

                private:
                    template <typename TKey>
                    struct CallbackCache {
                        typedef Internal::Magic::CompiledMap<Reactor, TKey, Internal::Reaction, Internal::Magic::LIST> cache;
                    };
                    std::vector<std::unique_ptr<NUClear::Reactor>> m_reactors;
            };

        protected:
            ThreadMaster threadmaster;
            ChronoMaster chronomaster;
            CacheMaster cachemaster;
            ReactorMaster reactormaster;
        public:
            ReactorController();

            void start();
            void shutdown();

            template <typename TData>
            auto get() -> decltype(cachemaster.get<TData>()) {
                return cachemaster.get<TData>();
            }
        
            template <typename TReactor>
            void install();
            
            template <typename TTrigger>
            void emit(TTrigger* data);

            // Chrono functions

            // Threading Functions

    };
}

#include "Reactor.h"
#include "ChronoMaster.ipp"
#include "CacheMaster.ipp"
#include "ReactorMaster.ipp"
#include "ReactorController.ipp"
#endif

