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
#include <queue>
#include <typeindex>
#include <mutex>
#include <map>
#include <unordered_map>
#include <iostream>
#include <string>

#include "nuclear_bits/DataFor.h"
#include "nuclear_bits/extensions/serialization/Serialization.h"
#include "nuclear_bits/threading/ThreadWorker.h"
#include "nuclear_bits/threading/TaskScheduler.h"
#include "nuclear_bits/metaprogramming/MetaProgramming.h"
#include "nuclear_bits/metaprogramming/TypeMap.h"
#include "nuclear_bits/metaprogramming/Sequence.h"
#include "nuclear_bits/ForwardDeclarations.h"
#include "nuclear_bits/LogLevel.h"

// Patch for std::make_unique in c++11 (should be fixed in c++14)
#if __cplusplus == 201103L
namespace std {
    template<typename T, typename... Args>
    std::unique_ptr<T> make_unique(Args&&... args) {
        return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
    }
}
#endif

namespace NUClear {
    // Declare our clock type
    using clock = std::chrono::high_resolution_clock;
    
    // We import our Meta Programming tools
    using namespace metaprogramming;
    
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
                Configuration() :
                    threadCount(4)
                    , networkName("default")
                    , networkGroup("NUClear")
                    , networkPort(7447) {}

                /// @brief The number of threads the system will use
                unsigned threadCount;
                
                /// @brief The name of the network we are connecting to
                std::string networkName;
                /// @brief The name of this PowerPlant within the networked PowerPlants
                std::string networkGroup;
                /// @brief The port to use when connecting to the network
                unsigned networkPort;
            };
        
        private:
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
                    PowerPlant* parent;
            };
        
            /**
             * @brief The ThreadMaster class is responsible for managing the thread pool and any service threads needed.
             *
             * @details
             *  TODO
             */
            class ThreadMaster : public BaseMaster {
                private:
                    // TODO when c++11 comes out in full, this can be replaced with a thread_local keyword variable
                    std::map<std::thread::id, const threading::ReactionTask*> currentTask;

                public:
                    /// @brief Construct a new ThreadMaster with our PowerPlant as context
                    ThreadMaster(PowerPlant* parent);

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
                    const threading::ReactionTask* getCurrentTask(std::thread::id threadId);

                    /**
                     * @brief TODO
                     *
                     * @details
                     *  TODO
                     *
                     * @param threadId  TODO
                     * @param task      TODO
                     */
                    void setCurrentTask(std::thread::id threadId, const threading::ReactionTask* task);

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
                    void submit(std::unique_ptr<threading::ReactionTask>&& task);
                
                    /**
                     * @brief Submits a service task to be executed when the system starts up (will run in its own thread)
                     *
                     * @details
                     *  TODO
                     *
                     * @param task The Service task to be executed with the system
                     */
                    void serviceTask(threading::ThreadWorker::ServiceTask task);
                
                private:
                    /// @brief A vector of the threads in the system
                    std::vector<std::unique_ptr<threading::ThreadWorker>> threads;
                    /// @brief A vector of the Service tasks to be started with the system
                    std::vector<threading::ThreadWorker::ServiceTask> serviceTasks;
                    /// @brief Our TaskScheduler that handles distributing task to the pool threads
                    threading::TaskScheduler scheduler;
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
                    using ValueCache = metaprogramming::TypeMap<CacheMaster, TData, TData>;
                
                    /// @brief This map stores the thread arguments when a function is called so that on an emit they can be retrieved.
                    std::unordered_map<std::thread::id, const threading::ReactionTask*> threadArgs;
                
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
                    void cache(std::shared_ptr<TData> cache);
                
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
                     *  TODO
                     *
                     * @tparam TData TODO
                     *
                     * @return TODO
                     */
                    template <typename TData>
                    auto get() -> decltype(Get<TData>::get(parent)) {
                        return Get<TData>::get(parent);
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
                    template <typename TData>
                    void emit(std::shared_ptr<TData> data);

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
                    template <typename TData>
                    void directEmit(std::shared_ptr<TData> data);

                    /**
                     * @brief 
                     *  Queues an emit to trigger on start() instead of
                     *  immediately.
                     */
                    template <typename TData>
                    void emitOnStart(std::shared_ptr<TData> data);

                    /**
                     * @brief TODO
                     *
                     * @details
                     *  TODO
                     *
                     * @tparam TReactor TODO
                     */
                    template <typename TReactor, enum LogLevel level = DEBUG>
                    void install();

                    void start();

                private:
                    /// @brief TODO
                    template <typename TKey>
                    using CallbackCache = metaprogramming::TypeList<Reactor, TKey, std::unique_ptr<threading::Reaction>>;

                    /// @brief TODO
                    std::vector<std::unique_ptr<NUClear::Reactor>> reactors;

                    std::queue<std::function<void ()>> deferredEmits;
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

        public:
            /// @brief TODO
            const Configuration configuration;
        protected:
            /// @brief TODO
            ThreadMaster threadmaster;
            /// @brief TODO
            CacheMaster cachemaster;
            /// @brief TODO
            ReactorMaster reactormaster;
        public:
            /**
             * @brief 
             *  Constructs a PowerPlant with the given configuration and provides access
             *  to argv for all reactors.
             *
             * @details
             *  If PowerPlant is constructed with argv and argv then a CommandLineArguments
             *  message will be emitted and available to all reactors.
             */
            PowerPlant(Configuration config = Configuration(), int argc = 0, const char *argv[] = nullptr);

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
             * @param task TODO
             */
            void addServiceTask(threading::ThreadWorker::ServiceTask task);

            /**
             * @brief TODO
             *
             * @details
             *  TODO
             *
             * @tparam TReactor TODO
             */
            template <typename TReactor, enum LogLevel level = DEBUG>
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
            void emit(std::unique_ptr<TData>&& data);
    };
}

// Include our Reactor.h first as the tight coupling between powerplant and reactor requires a specific include order
#include "nuclear_bits/Reactor.h"

// Include all of our implementation files (which use the previously included reactor.h)
#include "nuclear_bits/PowerPlant.ipp"
#include "nuclear_bits/CacheMaster.ipp"
#include "nuclear_bits/ReactorMaster.ipp"

// Include our built in extensions
#include "nuclear_bits/extensions/Chrono.h"
#include "nuclear_bits/extensions/Raw.h"
#include "nuclear_bits/extensions/Last.h"
#include "nuclear_bits/extensions/Networking.h"
#include "nuclear_bits/extensions/CommandLineArguments.h"

#endif

