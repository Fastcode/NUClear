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

#ifndef NUCLEAR_INTERNAL_COMMANDTYPES_COMMANDTYPES_H
#define NUCLEAR_INTERNAL_COMMANDTYPES_COMMANDTYPES_H

#include <string>
#include "NUClear/Internal/Reaction.h"

namespace NUClear {
    
    typedef std::chrono::high_resolution_clock clock;
    
    /**
     * @ingroup Options
     * @brief Enum to specify which of the priorities to use for a task
     */
    enum EPriority {
        /// @brief Run the task immediantly, if there are no available threads in the thread pool, create a new thread to run this task
        REALTIME,
        /// @brief Run this task before other tasks once there is a free thread
        HIGH,
        /// @brief Run this task at the normal priority
        DEFAULT,
        /// @brief Run this task when there is a free pool thread, and no other tasks to use it
        LOW
    };
    
    namespace Internal {
        
        // Forward declare SyncQueue
        struct SyncQueue;
        
        namespace CommandTypes {
            
            /**
             * @defgroup Wrappers
             *
             * @brief Wrappers are the base level types in an on statement. They encapsulate other options and flag their usage.
             *
             * @details
             *  The Wrappers are used in an on statement to catagorize the types they contain. For instance, the Trigger
             *  wrapper signifies that the types which are contained within it are used to trigger the running of the
             *  statement.
             */
            
            /**
             * @defgroup Options
             *
             * @brief Options are used within an Options wrapper to specify how a task will be run.
             *
             * @details
             *  Options are used on a task to specify how the task will be executed. They affect when and if the task
             *  will be scheduled to run.
             */
            
            /**
             * @defgroup SmartTypes
             *
             * @brief Smart types are types which when used within a Trigger or With statement, return different results.
             *
             * @details
             *  When a smart type is put within a trigger or with statement, the return type, data returned, and what it
             *  will trigger on can be changed. This can be used to access special features of the API and to make tasks
             *  which may be more difficult to accomplish easier.
             */
            
            /**
             * @brief This enum is used to modify in what way a packet is sent, either Local, Network or both
             *
             * @details
             *  When using the emit function, these options can be added in order to specify where the packet is to be
             *  sent. If nothing or LOCAL is specified, then it will only be send within this powerplant. If NETWORK is
             *  specified, then the data will be sent to other powerplants but not this one, if both LOCAL and NETWORK
             *  are chosen, then the event will propagate to both the network and local powerplants.
             */
            struct Scope {
                struct DIRECT;
                struct LOCAL;
                struct NETWORK;
            };
            
            /**
             * @ingroup Wrappers
             * @brief This is a wrapper class which is used to list the data types to trigger a callback on.
             *
             * @details
             *  This class is used in the on binding to specify which data types are to be used trigger a callback. It
             *  works under and logic for multiple types. When all of the types have been emitted at least once since
             *  the last time this event was triggered, this event is triggered again.
             *
             * @tparam TTriggers the datatypes to trigger a callback on
             */
            template <typename... TTriggers>
            struct Trigger { Trigger() = delete; ~Trigger() = delete; };
            
            /**
             * @ingroup Wrappers
             * @brief This is a wrapper class which is used to list the data types to trigger a callback on.
             *
             * @details
             *  This class is used in the on binding to specify additional data which is used in a call, but does not
             *  trigger the call to run. When the call is run, the latest version of the data available will be obtained.
             *  from the cache, and used as a paramter in this function call.
             *
             * @tparam TWiths the datatypes to get from the cache and use in the callback.
             */
            template <typename... TWiths>
            struct With { With() = delete; ~With() = delete; };
            
            /**
             * @ingroup Wrappers
             * @brief This is a wrapper class which is used to hold the options which specify how this callback will run.
             *
             * @details
             *  This class is used to hold data types which signify execution options for the callback. These options
             *  alter the way the task is scheduled and executed.
             *
             * @tparam TOptions The list of options that to be used in the callback
             */
            template <typename... TOptions>
            struct Options { Options() = delete; ~Options() = delete; };
            
            /**
             * @ingroup Options
             * @brief This option sets the priority of the task.
             *
             * @attention
             *  Due to the way the REALTIME option works in the system, it should be used sparingly as overuse
             *  will cause the system to spawn excessive threads and slowdown
             *
             * @details
             *  The priority option sets at which priority to allocate this task a thread in the thread pool. The options
             *  LOW DEFAULT and HIGH act this way, choosing tasks which have a higher priority before lower ones (unless
             *  their sync group is disabled). However Realtime works differently, ignoring the Sync group and ensuring
             *  that the task is executed immediantly. If there is no thread pool thread ready to execute this task. The
             *  system will spawn a new thread to ensure it happens.
             *
             * @tparam P the priority to run the task at (LOW DEFAULT HIGH and REALTIME)
             */
            template <enum EPriority P>
            struct Priority { Priority() = delete; ~Priority() = delete; };
            
            /**
             * @ingroup Options
             * @brief This option sets the Synchronization group of the task
             *
             * @details
             *  The synchronization group of a task is a compile time mutex which will allow only a single task from
             *  each distinct execution task to execute at a time. For example, if two tasks both had Sync<int> then only
             *  one of those tasks would execute at a time.
             *
             * @tparam TSync the type with which to synchronize on
             */
            template <typename TSync>
            struct Sync {
                friend class Reaction;
                
                Sync() = delete;
                ~Sync() = delete;
                private:
                static const std::shared_ptr<Internal::SyncQueue> queue;
            };
            
            /**
             * @ingroup Options
             * @brief This option sets the Single instance status of the task
             *
             * @details
             *  If a task is declared as being Single, then that means that only a single instance of the task can be
             *  in the system at any one time. If the task is triggered again while an existing task is either in the
             *  queue or is still executing, then this new task will be ignored.
             */
            struct Single { Single() = delete; ~Single() = delete; };
            
            /**
             * @ingroup SmartTypes
             * @brief This type is emitted when the system is first started up.
             *
             * @details
             *  This type is emitted when the system first starts up, after reactors have been installed. This is
             *  used if a reactor needs to use information provided by other reactors constructors.
             */
            struct Initialize {};
            
            /**
             * @ingroup SmartTypes
             * @brief This type is emitted when the system starts shutting down.
             *
             * @details
             *  Once this type is emitted, all existing tasks within the system are completed including the shutdown
             *  tasks, Any new emit events will not be processed and all new tasks will be ignored. Once all tasks are
             *  finish the system will terminate.
             */
            struct Shutdown {};
            
            /**
             * @ingroup SmartTypes
             * @brief A special flag type which specifies that this reaction should trigger at the given rate
             *
             * @details
             *  This type is used in a Trigger statement to specify that the given reaction should trigger at the rate
             *  set by this type. For instance, if the type was specified as shown in the following example
             *  @code on<Trigger<Every<2, std::chrono::seconds>> @endcode
             *  then the callback would execute every 2 seconds. This type simply needs to exist in the trigger for the
             *  correct timing to be called.
             *
             * @attention Note that the period which is used to measure the ticks in must be equal to or greater then
             *  clock::duration or the program will not compile
             *
             * @tparam ticks the number of ticks of a paticular type to wait
             * @tparam period a type of duration (e.g. std::chrono::seconds) to measure the ticks in, Defaults to Milliseconds
             *
             * @return Returns the time the every was emitted as an clock::time_point
             */
            template <int ticks, class period = std::chrono::milliseconds>
            struct Every {
                Every(clock::time_point time) : time(time) {}
                const clock::time_point time;
            };
            
            /**
             * @ingroup SmartTypes
             * @brief This is a special type which is used to get a list of the last n emissions of type TData
             *
             * @details
             *  This class is used to get the last n data events of a type which have been emitted. These are returned
             *  as const shared pointers in a vector, which can then be used to access the data. The data is returned in
             *  descending order, i.e. the most recent event is first in the result
             *
             * @tparam n        The number of events to get
             * @tparam TData    The datatype of object to get
             *
             * @return  Returns a vector of shared_ptr with the signature std::vector<std::shared_ptr<TData>>
             */
            template <int n, typename TData>
            struct Last { Last() = delete; ~Last() = delete; };
            
            /**
             * @ingroup SmartTypes
             * @brief This is a special type which is used to get data that is sent over the network from other NUClear
             *  instances
             *
             * @details
             *  This class is used to get a paticular datatype from the network. Once this object is collected the
             *  original data and the sender can be accessed from this object.
             *
             * @tparam TType    The datatype of the networked data to get
             *
             * @return Returns an object of type Network<TType> which contains a sender and the data
             */
            template <typename TType>
            struct Network {
                Network(std::string sender, std::unique_ptr<TType>&& data) : sender(sender), data(std::move(data)) {};
                const std::string sender;
                const std::unique_ptr<const TType> data;
            };
        }
    }
}

#endif
