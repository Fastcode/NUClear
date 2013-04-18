//
//  CompiledCache.h
//  NUBots
//
//  Created by Trent Houliston on 18/04/13.
//  Copyright (c) 2013 University of Newcastle. All rights reserved.
//

#ifndef NUCLEAR_INTERNAL_MAGIC_COMPILEDCACHE_H
#define NUCLEAR_INTERNAL_MAGIC_COMPILEDCACHE_H

#include <deque>

namespace NUClear {
namespace Internal {
namespace Magic {
    
    /**
     * @brief This exception is thrown when there is no data in the cache to return.
     */
    struct NoDataException {};
    
    /**
     * @brief This enum is used to select what kind of cache is to be used for the data
     */
    enum CacheType {
        /// @see CompiledCache <TCacheID, TData, SINGLE>
        SINGLE,
        /// @see CompiledCache <TCacheID, TData, QUEUE>
        QUEUE,
        /// @see CompiledCache <TCacheID, TData, LIST>
        LIST
    };
    
    /**
     * @brief
     *  This class provides a compile time map. This allows the compiler to optimize map access if the types are known
     *  at compile time.
     *
     * @details
     *  This cache is accessed by template paramters, because of this when the compiler compiles this map. It can resolve
     *  each of the map accesses into a direct function call. This allows the map to be looked up at compile time and
     *  optimized to very efficent code. There are several variations of the Cache provided through the CacheType parameter
     *  the operation of each of these is described in their individual documentation.
     *
     * @attention
     *  Note that because this is an entirely static class, if two maps with the same TCacheID are used, they access the
     *  same map
     *
     * @tparam TCacheID     A typename identifier to ensure that a new cache is generated for each usage
     * @tparam TData        The type of data which is being stored in this cache
     * @tparam Type         The type of cache which is being created
     */
    template <typename TCacheID, typename TData, enum CacheType Type>
    class CompiledCache;
    
    /**
     * @brief The simplest and fastest cache format, It stores a single value and returns it when requested later.
     *
     * @details
     *  This cache stores a single value in it's cache when the Cache function is called, and when get is later called
     *  this object will be returned
     *
     * @see CompiledCache
     */
    template <typename TCacheID, typename TData>
    class CompiledCache <TCacheID, TData, SINGLE> {
        private:
            /// @brief Deleted constructor as this class is a static class.
            CompiledCache() = delete;
            /// @brief Deleted destructor as this class is a static class.
            ~CompiledCache() = delete;
            /// @brief the cache variable where the data is stored.
            static std::shared_ptr<TData> m_cache;
            
        public:
            /**
             * @brief Caches the passed value in this cache.
             *
             * @param data a pointer to the data to be cached (the cache takes ownership)
             */
            static void cache(TData* data) {
                m_cache = std::shared_ptr<TData>(data);
            };
        
            /**
             * @brief Gets the value that was previously cached.
             *
             * @return a shared_ptr to the data that was previously cached
             *
             * @throws NoDataException if there is no data that was previously cached
             */
            static std::shared_ptr<TData> get() {
                //If the pointer is not a nullptr
                if(*m_cache) {
                    return m_cache;
                }
                else {
                    throw NoDataException();
                }
            }
    };
    
    /**
     * @brief
     *  This cache acts as a ring buffer of data, with the ability to get both the most recent data element as well
     *  as the previous n elements.
     *
     * @details
     *  This cache acts as a ring buffer. When data is cached, it will remove the oldest element that is cached up to
     *  the capacity of the cache. The list of previous elements are accessable using the get(int) method.
     *
     * @see CompiledCache
     */
    template <typename TCacheID, typename TData>
    class CompiledCache <TCacheID, TData, QUEUE> {
        private:
            /// @brief Deleted constructor as this class is a static class
            CompiledCache() = delete;
            /// @brief Deleted destructor as this class is a static class
            ~CompiledCache() = delete;
            /// @brief the number of previous elements that we store
            static int m_capacity;
            /// @brief the cache where we store cached elements
            static std::deque<std::shared_ptr<TData>> m_cache;
            
        public:
            /**
             * @brief increases the minimum capacity of stored objects to the passed integer.
             *
             * @param num the minimum number of previous elements to cache
             */
            static void minCapacity(int num) {
                for(;m_capacity < num; ++m_capacity) {
                    m_cache.emplace_back(nullptr);
                }
            }
        
            /**
             * @brief Caches the passed pointer in our storage, removing the oldest element
             *
             * @param data a pointer to the data to cache (the cache takes ownership of it)
             */
            static void cache(TData* data) {
                m_cache.pop_back();
                m_cache.push_front(std::shared_ptr<TData>(data));
            }
        
            /**
             * @brief Gets the value that was previously cached.
             *
             * @return a shared_ptr to the most recent data that was cached
             *
             * @throws NoDataException if there is no data that was previously cached
             */
            static std::shared_ptr<TData> get() {
                if(!m_cache.empty() && m_cache.front() != nullptr) {
                    return m_cache.front();
                }
                else {
                    throw NoDataException();
                }
            };
        
            /**
             * @brief Gets the last n elements in the cache
             *
             * @return a vector with the last n elements that have been cached
             *
             * @attention make sure that minCapacity has been called with at least this length or a segfault will occur
             */
            static std::shared_ptr<std::vector<std::shared_ptr<const TData>>> get(int length) {
                return std::shared_ptr<std::vector<std::shared_ptr<const TData>>>(new std::vector<std::shared_ptr<const TData>>(std::begin(m_cache), std::begin(m_cache) + length));
            };
    };
    
    
    //Initialize the capacity of a QUEUE cache
    template <typename TCacheID, typename TData>
    int CompiledCache<TCacheID, TData, QUEUE>::m_capacity = 1;
    
    //Initialize the deque object of a QUEUE cache
    template <typename TCacheID, typename TData>
    std::deque<std::shared_ptr<TData>> CompiledCache<TCacheID, TData, QUEUE>::m_cache = std::deque<std::shared_ptr<TData>>(1);
    
    //Initialize the vector of a LIST cache
    //template <typename TCacheID, typename TData>
    //std::deque<std::shared_ptr<TData>> CompiledCache<TCacheID, TData, LIST>::m_cache = std::vector<std::shared_ptr<TData>>(1);
}
}
}

#endif
