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
#include <map>
#include <iostream>
#include "Internal/Every.h"
#include "Internal/ThreadWorker.h"
#include "Internal/TaskScheduler.h"

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

            class ChronoMaster : public BaseMaster {
                public:
                    ChronoMaster(ReactorController* parent);
                    ~ChronoMaster();

                    void run();

                    template <int ticks, class period>
                    void add();
                private:
                    /// @brief this class holds the callbacks to emit events, as well as when to emit these events.
                    struct Step {
                        std::chrono::nanoseconds step;
                        std::chrono::nanoseconds next;
                        std::vector<std::function<void ()>> callbacks;
                    };
                
                    /// @brief If the system should continue to execute or if it should stop
                    bool m_execute;
                
                    /// @brief A vector of steps containing the callbacks to execute, is sorted regularly to maintain the order
                    std::vector<std::unique_ptr<Step>> m_steps;
                
                    /// @brief A list of types which have already been loaded (to avoid duplication)
                    std::set<std::type_index> m_loaded;
            };

            class ReactorMaster : public BaseMaster {
                public:
                    ReactorMaster(ReactorController* parent);

                    template <typename TTrigger>
                    void emit(TTrigger* data);

                    template <typename TData>
                    std::shared_ptr<TData> get();

                    template <typename TReactor>
                    void install();

                    template <typename TTrigger>
                    void subscribe(NUClear::Reactor* reactor);
                private:
                    template <typename TTrigger>
                    void cache(TTrigger* data);

                    template <typename TTrigger>
                    void notifyReactors();

                    template <typename TTrigger>
                    std::set<NUClear::Reactor*>& getReactorBindings();

                    std::vector<std::unique_ptr<NUClear::Reactor>> m_reactors;
                    std::map<std::type_index, std::shared_ptr<void> > m_cache;
                    std::map<std::type_index, std::set<NUClear::Reactor*> > m_reactorBindings;
            };

            class ThreadMaster : public BaseMaster {
                public:
                    ThreadMaster(ReactorController* parent);

                    void start();
                    void submit(std::unique_ptr<Internal::ReactionTask>&& task);
                private:
                    std::map<std::thread::id, std::unique_ptr<Internal::ThreadWorker>> m_threads;

                    Internal::TaskScheduler m_scheduler;
                    int numThreads = 4;
            };
        protected:
            ChronoMaster chronomaster;
            ReactorMaster reactormaster;
            ThreadMaster threadmaster;
        public:
            ReactorController();

            void start();

            template <typename TData>
            std::shared_ptr<TData> get();
        
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
#include "ReactorMaster.ipp"
#include "ReactorController.ipp"
#endif

