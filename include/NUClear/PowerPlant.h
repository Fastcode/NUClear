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

#ifndef NUCLEAR_POWERPLANT_H
#define NUCLEAR_POWERPLANT_H

#include <memory>
#include <set>
#include <thread>
#include <vector>
#include <typeindex>
#include <mutex>
#include <map>
#include <unordered_map>
#include <iostream>
#include <string>
#include <sstream>

#include <zmq.hpp>

#include "NUClear/Networking/Serialization.h"
#include "NUClear/NetworkMessage.pb.h"
#include "NUClear/Internal/ThreadWorker.h"
#include "NUClear/Internal/TaskScheduler.h"
#include "NUClear/Internal/CommandTypes/CommandTypes.h"
#include "NUClear/Internal/Magic/TypeMap.h"
#include "NUClear/Internal/Magic/Sequence.h"

namespace NUClear {
    
    class Reactor;
    class PowerPlant {
        
        public:
            struct Configuration {
                unsigned threadCount = 4;
                
                std::string networkName = "default";
                std::string networkGroup = "NUClear";
                unsigned networkPort = 7447;
            };
        
        private:
            friend class Reactor;

            class BaseMaster {
                public:
                    BaseMaster(PowerPlant* parent); 
                protected:
                    PowerPlant* m_parent;
            };
        
            class ThreadMaster : public BaseMaster {
                public:
                    
                    ThreadMaster(PowerPlant* parent);
                    
                    void start();
                    void shutdown();
                    void submit(std::unique_ptr<Internal::Reaction::Task>&& task);
                    void internalTask(Internal::ThreadWorker::InternalTask task);
                private:
                    std::unordered_map<std::thread::id, std::unique_ptr<Internal::ThreadWorker>> m_threads;
                    std::vector<Internal::ThreadWorker::InternalTask> m_internalTasks;
                    Internal::TaskScheduler m_scheduler;
            };

            class ChronoMaster : public BaseMaster {
                public:
                    ChronoMaster(PowerPlant* parent);
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
                    using ValueCache = Internal::Magic::TypeBuffer<CacheMaster, TData, TData>;
                
                    std::map<void*, std::vector<std::pair<std::type_index, std::shared_ptr<void>>>> m_linkedCache;
                
                    std::unordered_map<std::thread::id, std::vector<std::pair<std::type_index, std::shared_ptr<void>>>> m_threadArgs;
                
                public:
                    CacheMaster(PowerPlant* parent);
                    ~CacheMaster();
                
                    template <typename TData>
                    void cache(TData* cache);
                
                    template <typename TData>
                    struct Get;
                
                    template <typename TData>
                    auto get() -> decltype(Get<TData>()()) {
                        return Get<TData>()();
                    }
                
                    template <int num, typename TData>
                    void ensureCache();
                
                    void setThreadArgs(std::thread::id threadId, std::vector<std::pair<std::type_index, std::shared_ptr<void>>>&& args);
                    std::vector<std::pair<std::type_index, std::shared_ptr<void>>> getThreadArgs(std::thread::id threadId);
                
                    void linkCache(void* data, std::vector<std::pair<std::type_index, std::shared_ptr<void>>> args);
                
                    template <typename... TData, typename TElement>
                    TElement doFill(std::tuple<TData...> data, TElement element);
                
                    template <typename... TData, typename TElement, int index>
                    std::shared_ptr<TElement> doFill(std::tuple<TData...> data, Internal::CommandTypes::Linked<TElement, index>);
                
                    template <typename... TData, int... S>
                    auto fill(Internal::Magic::Sequence<S...> s, std::tuple<TData...> data) -> decltype(std::make_tuple(doFill(data, std::get<S>(data))...)) {
                        return std::make_tuple(doFill(data, std::get<S>(data))...);
                    }
                
                    template <typename... TData>
                    auto fill(std::tuple<TData...> data) -> decltype(fill(typename Internal::Magic::GenerateSequence<sizeof...(TData)>::type(), data)) {
                        return fill(typename Internal::Magic::GenerateSequence<sizeof...(TData)>::type(), data);
                    }
            };

            class ReactorMaster : public BaseMaster {
                public:
                    ReactorMaster(PowerPlant* parent);

                    template <typename TTrigger>
                    void emit(TTrigger* data);
                    
                    template <typename TReactor>
                    void install();

                private:
                    template <typename TKey>
                    using CallbackCache = Internal::Magic::TypeList<Reactor, TKey, Internal::Reaction>;

                    std::vector<std::unique_ptr<NUClear::Reactor>> m_reactors;
            };
        
            class NetworkMaster : public BaseMaster {
                public:
                    NetworkMaster(PowerPlant* parent);
                
                    // Serializes a data and pushes it out over the network
                    template<typename TType>
                    void emit(TType* data);
                
                    // Adds a type to deserialize (finds a deserializer, subscribes to this message)
                    template<typename TType>
                    void addType();
                
                private:
                    // Starts up a new thread that receives network packets and listens for new clients
                    void run();
                
                    void kill();
                
                    // Converts a string into our epgm address for our network
                    static std::string addressForName(std::string name, unsigned port);
                
                    std::unordered_map<Networking::Hash, std::function<void(const std::string, const void*, const size_t)>> m_deserialize;
                    volatile bool m_running;
                    zmq::context_t m_context;
                    zmq::socket_t m_pub;
                    zmq::socket_t m_sub;
            };

        protected:
            const Configuration configuration;
            ThreadMaster threadmaster;
            ChronoMaster chronomaster;
            CacheMaster cachemaster;
            ReactorMaster reactormaster;
            NetworkMaster networkmaster;
        public:
            PowerPlant();
            PowerPlant(Configuration config);

            void start();
            void shutdown();

            template <typename TData>
            auto get() -> decltype(cachemaster.get<TData>()) {
                return cachemaster.get<TData>();
            }
        
            template <typename TReactor>
            void install();
            
            template <enum Internal::CommandTypes::Scope... TScopes, typename TData>
            void emit(TData* data);
    };
}

#include "NUClear/Reactor.h"
#include "NUClear/PowerPlant.ipp"
#include "NUClear/ChronoMaster.ipp"
#include "NUClear/CacheMaster.ipp"
#include "NUClear/ReactorMaster.ipp"
#include "NUClear/NetworkMaster.ipp"
#endif

