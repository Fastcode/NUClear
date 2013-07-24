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
#include "NUClear/Internal/Magic/MetaProgramming.h"
#include "NUClear/Internal/Magic/TypeMap.h"
#include "NUClear/Internal/Magic/Sequence.h"

namespace NUClear {
    
    // Forward declare reactor
    class Reactor;
    
    // Declare our clock type
    using clock = std::chrono::high_resolution_clock;
    
    // We import our Meta Programming tools
    using namespace Internal::Magic::MetaProgramming;
    
    /**
     * @brief The PowerPlant is the core of a NUClear system. It holds all Reactors in it and manages their communications.
     *
     * @details
     *  TODO
     *
     * @author Trent Houliston
     * @author Jake Woods
     */
    class PowerPlant {
        // Reactors and PowerPlants are very tightly linked
        friend class Reactor;
        
        public:
            /**
             * @brief TODO
             *
             * @details
             *  TODO
             */
            struct Configuration {
                /// @brief The number of threads the system will use
                unsigned threadCount = 4;
                
                /// @brief The name of the network we are connecting to
                std::string networkName = "default";
                /// @brief The name of this PowerPlant within the networked PowerPlants
                std::string networkGroup = "NUClear";
                /// @brief The port to use when connecting to the network
                unsigned networkPort = 7447;
            };
        
        private:
            
            /// @brief This boolean is used to aid the static content in knowing if the PowerPlant still exists
            static volatile bool running;
        
            /**
             * @brief The base master class is used as a base for all of the other masters.
             *
             * @details
             *  TODO
             */
            class BaseMaster {
                public:
                    /// @brief Construct a new BaseMaster with our PowerPlant as context
                    BaseMaster(PowerPlant* parent); 
                protected:
                    /// @brief The PowerPlant that this belongs to
                    PowerPlant* m_parent;
            };
        
            /**
             * @brief The ThreadMaster class is responsible for managing the thread pool and any service threads needed.
             *
             * @details
             *  TODO
             */
            class ThreadMaster : public BaseMaster {
                public:
                    /// @brief Construct a new ThreadMaster with our PowerPlant as context
                    ThreadMaster(PowerPlant* parent);
                
                    /**
                     * @brief Starts up the ThreadMaster initiating all service threads and pool threads.
                     * 
                     * @details
                     *  This will start up the entire system, beginning all threads and running the needed code. It will
                     *  block the thread that calls it until the system is shut down. This method should only be called
                     *  from the main thread. This will prevent the main thread ending which deletes all statics (which
                     *  are important for us).
                     */
                    void start();
                
                    /**
                     * @brief 
                     *  Shuts down the ThreadMaster, it will terminate all of the service and pool threads, and then
                     *  releases the main thread to terminate the system.
                     */
                    void shutdown();
                
                    /**
                     * @brief Submits a new task to the ThreadPool to be queued and then executed.
                     *
                     * @details
                     *  TODO
                     *
                     * @param task The Reactor task to be executed in the thread pool
                     */
                    void submit(std::unique_ptr<Internal::Reaction::Task>&& task);
                
                    /**
                     * @brief Submits a service task to be executed when the system starts up (will run in its own thread)
                     *
                     * @details
                     *  TODO
                     *
                     * @param task The Service task to be executed with the system
                     */
                    void serviceTask(Internal::ThreadWorker::ServiceTask task);
                
                private:
                    /// @brief A vector of the threads in the system
                    std::vector<std::unique_ptr<Internal::ThreadWorker>> m_threads;
                    /// @brief A vector of the Service tasks to be started with the system
                    std::vector<Internal::ThreadWorker::ServiceTask> m_serviceTasks;
                    /// @brief Our TaskScheduler that handles distributing task to the pool threads
                    Internal::TaskScheduler m_scheduler;
            };

            /**
             * @brief The Chronomaster is a Service thread that manages the Every class time emissions.
             *
             * @details
             *  TODO
             */
            class ChronoMaster : public BaseMaster {
                public:
                    /// @brief Construct a new ThreadMaster with our PowerPlant as context
                    ChronoMaster(PowerPlant* parent);

                    /**
                     * @brief Adds a new timeperiod to count and send out events for.
                     *
                     * @details
                     *  TODO
                     *
                     * @tparam ticks    The number of ticks of the period to wait between emitting events
                     * @tparam period   The period that the ticks are measured in (a type of std::chrono::duration)
                     */
                    template <int ticks, class period>
                    void add();
                
                private:
                    /// @brief this class holds the callbacks to emit events, as well as when to emit these events.
                    struct Step {
                        /// @brief the size our step is measured in (the size our clock uses)
                        clock::duration step;
                        /// @brief the time at which we need to emit our next event
                        clock::time_point next;
                        /// @brief the callbacks to emit for this time (e.g. 1000ms and 1second will trigger on the same tick)
                        std::vector<std::function<void (clock::time_point)>> callbacks;
                    };
                
                    /// @brief Our Run method for the task scheduler, starts the Every events emitting
                    void run();
                    /// @brief Our Kill method for the task scheduler, shuts down the chronomaster and stops emitting events
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
        
            /**
             * @brief The CacheMaster is responsible for handling all of the data storage in the system.
             *
             * @details
             *  TODO
             *
             * @attention
             *  This CacheMaster uses static variables to enhance its speed, this means that you cannot have more then
             *  one PowerPlant in an executable without resolving this.
             */
            class CacheMaster : public BaseMaster {
                private:
                    /// @brief This Value cache is a special Static type buffer that allows compile time lookup of types.
                    template <typename TData>
                    using ValueCache = Internal::Magic::TypeBuffer<CacheMaster, TData, TData>;
                
                    /// @brief This map contains the linked cache, it maps objects to the shared_ptrs that created them
                    std::map<void*, std::vector<std::pair<std::type_index, std::shared_ptr<void>>>> m_linkedCache;
                
                    /// @brief This map stores the thread arguments when a function is called so that on an emit they can be retrieved.
                    std::unordered_map<std::thread::id, std::vector<std::pair<std::type_index, std::shared_ptr<void>>>> m_threadArgs;
                
                public:
                    /// @brief Construct a new CacheMaster with our PowerPlant as context
                    CacheMaster(PowerPlant* parent);
                
                    /**
                     * @brief Stores the passed data in our cache so that it can be retrieved.
                     *
                     * @details
                     *  TODO The Cache takes ownership of this pointer
                     *
                     * @tparam TData the type of data we are caching
                     *
                     * @param cache the data that we are caching
                     */
                    template <typename TData>
                    void cache(TData* cache);
                
                    /**
                     * @brief Sets the minimum number of previous elements to store for each datatype.
                     *
                     * @details
                     *  This method sets the minimum number of previous elements to store before they are deleted from
                     *  the cache. It will keep the largest number that this is called with for each datatype.
                     *
                     * @tparam num     the number of elements to maintain
                     * @tparam TData   the type of data to maintain the elements for
                     */
                    template <int num, typename TData>
                    void ensureCache();
                
                    /**
                     * @brief
                     *  This datatype is used for extension of getting data, by default it will get the most recent
                     *  element from the cache.
                     *
                     * @details
                     *  TODO give code example of an extension
                     *
                     * @tparam TData the datatype that is mentioned in the trigger (does not have to be the return type)
                     */
                    template <typename TData>
                    struct Get;
                
                    /**
                     * @brief TODO
                     *
                     * @details
                     *  TODO give code example of an extension
                     *
                     * @tparam TData        TODO
                     * @tparam TElements    TODO
                     */
                    template <typename TData, typename... TElements>
                    struct Fill;
                
                    /**
                     * @brief TODO
                     *
                     * @details
                     *  TODO
                     *
                     * @tparam TData TODO
                     *
                     * @return TODO
                     */
                    template <typename TData>
                    auto get() -> decltype(Get<TData>::get(m_parent)) {
                        return Get<TData>::get(m_parent);
                    }
                    
                    /**
                     * @brief TODO
                     *
                     * @details
                     *  TODO
                     *
                     * @param threadId  TODO
                     *
                     * @return TODO
                     */
                    std::vector<std::pair<std::type_index, std::shared_ptr<void>>> getThreadArgs(std::thread::id threadId);
                    
                    /**
                     * @brief TODO
                     *
                     * @details
                     *  TODO
                     *
                     * @param threadId  TODO
                     * @param args      TODO
                     */
                    void setThreadArgs(std::thread::id threadId, std::vector<std::pair<std::type_index, std::shared_ptr<void>>>&& args);
                    
                    /**
                     * @brief TODO
                     *
                     * @details
                     *  TODO
                     *
                     * @param data TODO
                     * @param args TODO
                     */
                    void linkCache(void* data, std::vector<std::pair<std::type_index, std::shared_ptr<void>>> args);
                
                    /**
                     * @brief TODO
                     *
                     * @details
                     *  TODO
                     *
                     * @tparam TData    TODO
                     * @tparam S        TODO
                     *
                     * @param data
                     *
                     * @return TODO
                     */
                    template <typename... TData, int... S>
                    auto fill(Internal::Magic::Sequence<S...>, std::tuple<TData...> data)
                    -> decltype(std::make_tuple(Fill<typename std::remove_reference<decltype(std::get<S>(data))>::type, TData...>::fill(m_parent, std::get<S>(data), data)...)) {
                        return std::make_tuple(Fill<typename std::remove_reference<decltype(std::get<S>(data))>::type, TData...>::fill(m_parent, std::get<S>(data), data)...);
                    }
                
                    /**
                     * @brief TODO
                     *
                     * @details
                     *  TODO
                     *
                     * @tparam TData TODO
                     *
                     * @param data TODO
                     *
                     * @return TODO
                     */
                    template <typename... TData>
                    auto fill(std::tuple<TData...> data) -> decltype(fill(typename Internal::Magic::GenerateSequence<sizeof...(TData)>::type(), data)) {
                        return fill(typename Internal::Magic::GenerateSequence<sizeof...(TData)>::type(), data);
                    }
            };
        
            /**
             * @brief 
             *
             * @details
             *  TODO
             *
             * @attention
             *  This ReactorMaster uses static variables to enhance its speed, this means that you cannot have more then
             *  one PowerPlant in an executable without resolving this.
             */
            class ReactorMaster : public BaseMaster {
                public:
                    /**
                     * @brief Constructs a new ReactorMaster which is held in the PowerPlant that created it.
                     */
                    ReactorMaster(PowerPlant* parent);

                    /**
                     * @brief TODO
                     *
                     * @details
                     *  TODO
                     *
                     * @tparam TTrigger TODO
                     *
                     * @param data TODO
                     */
                    template <typename TTrigger>
                    void emit(TTrigger* data);
                    
                    /**
                     * @brief TODO
                     *
                     * @details
                     *  TODO
                     *
                     * @tparam TReactor TODO
                     */
                    template <typename TReactor>
                    void install();

                private:
                    /// @brief TODO
                    template <typename TKey>
                    using CallbackCache = Internal::Magic::TypeList<Reactor, TKey, Internal::Reaction>;

                    /// @brief TODO
                    std::vector<std::unique_ptr<NUClear::Reactor>> m_reactors;
            };
        
            class NetworkMaster : public BaseMaster {
                public:
                    /// @brief TODO
                    NetworkMaster(PowerPlant* parent);
                
                    /**
                     * @brief TODO
                     *
                     * @details
                     *  TODO
                     *
                     * @tparam TData
                     *
                     * @param data
                     */
                    template<typename TData>
                    void emit(TData* data);
                
                    /**
                     * @brief TODO
                     *
                     * @details
                     *  TODO
                     *
                     * @tparam TType TODO
                     */
                    template<typename TType>
                    void addType();
                
                private:
                    /**
                     * @brief TODO
                     *
                     * @details
                     *  TODO
                     */
                    void run();
                
                    /**
                     * @brief TODO
                     *
                     * @details
                     *  TODO
                     */
                    void kill();
                
                    /**
                     * @brief TODO
                     *
                     * @details
                     *  TODO
                     *
                     * @param name
                     * @param port
                     */
                    static std::string addressForName(const std::string name, const unsigned port);
                
                    /// @brief TODO
                    std::unordered_map<Networking::Hash, std::function<void(const std::string, std::string)>> m_deserialize;
                    /// @brief TODO
                    volatile bool m_running;
                    /// @brief TODO
                    std::mutex m_send;
                    /// @brief TODO
                    zmq::context_t m_context;
                    /// @brief TODO
                    zmq::socket_t m_pub;
                    /// @brief TODO
                    zmq::socket_t m_termPub;
                    /// @brief TODO
                    zmq::socket_t m_sub;
            };
        
            /**
             * @brief TODO
             *
             * @details
             *  TODO
             *
             * @tparam THandler
             * @tparam TData
             */
            template <typename THandler, typename TData>
            struct Emit;

        protected:
            /// @brief TODO
            const Configuration configuration;
            /// @brief TODO
            ThreadMaster threadmaster;
            /// @brief TODO
            ChronoMaster chronomaster;
            /// @brief TODO
            CacheMaster cachemaster;
            /// @brief TODO
            ReactorMaster reactormaster;
            /// @brief TODO
            NetworkMaster networkmaster;
        public:
            /**
             * @brief TODO
             *
             * @details
             *  TODO
             */
            PowerPlant();
        
            /**
             * @brief TODO
             *
             * @details
             *  TODO
             *
             * @param config TODO
             */
            PowerPlant(Configuration config);

            /**
             * @brief TODO
             *
             * @details
             *  TODO
             */
            void start();
        
            /**
             * @brief TODO
             *
             * @details
             *  TODO
             */
            void shutdown();
        
            /**
             * @brief TODO
             *
             * @details
             *  TODO
             *
             * @tparam TReactor TODO
             */
            template <typename TReactor>
            void install();
        
            /**
             * @brief TODO
             *
             * @details
             *  TODO
             *
             * @tparam THandlers    TODO
             * @tparam TData        TODO
             *
             * @param data TODO
             */
            template <typename... THandlers, typename TData>
            void emit(TData* data);
    };
}

// Include our Reactor.h first as the tight coupling between powerplant and reactor requires a specific include order
#include "NUClear/Reactor.h"

// Include all of our implementation files (which use the previously included reactor.h)
#include "NUClear/PowerPlant.ipp"
#include "NUClear/ChronoMaster.ipp"
#include "NUClear/CacheMaster.ipp"
#include "NUClear/ReactorMaster.ipp"
#include "NUClear/NetworkMaster.ipp"

// Include our built in extensions
#include "NUClear/Internal/Extensions/Every.h"
#include "NUClear/Internal/Extensions/Last.h"
#include "NUClear/Internal/Extensions/Linked.h"
#include "NUClear/Internal/Extensions/Network.h"

#endif

