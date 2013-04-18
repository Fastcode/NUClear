#ifndef NUCLEAR_INTERNAL_MAGIC_COMPILEDMAP_H
#define NUCLEAR_INTERNAL_MAGIC_COMPILEDMAP_H

#include <deque>
#include <vector>

namespace NUClear {
namespace Internal {
namespace Magic {
    
    /**
     * @brief This exception is thrown when there is no data in the map to return.
     */
    struct NoDataException {};
    
    /**
     * @brief This enum is used to select what kind of map is to be used for the data
     */
    enum MapType {
        /// @see CompiledMap <TMapID, TData, SINGLE>
        SINGLE,
        /// @see CompiledMap <TMapID, TData, QUEUE>
        QUEUE,
        /// @see CompiledMap <TMapID, TData, LIST>
        LIST
    };
    
    /**
     * @brief
     *  This class provides a compile time map. This allows the compiler to optimize map access if the types are known
     *  at compile time.
     *
     * @details
     *  This map is accessed by template paramters, because of this when the compiler compiles this map. It can resolve
     *  each of the map accesses into a direct function call. This allows the map to be looked up at compile time and
     *  optimized to very efficent code. There are several variations of the Map provided through the MapType parameter
     *  the operation of each of these is described in their individual documentation.
     *
     * @attention
     *  Note that because this is an entirely static class, if two maps with the same TMapID are used, they access the
     *  same map
     *
     * @tparam TMapID     A typename identifier to ensure that a new map is generated for each usage
     * @tparam TKey         The key being used to access the data
     * @tparam TValue       The type of data which is being stored in this map
     * @tparam Type         The type of map which is being created
     */
    template <typename TMapID, typename TKey, typename TValue, enum MapType Type>
    class CompiledMap;
    
    /**
     * @brief The simplest and fastest map format, It stores a single value and returns it when requested later.
     *
     * @details
     *  This map stores a single value in it's store when the set function is called, and when get is later called
     *  this object will be returned
     *
     * @see CompiledMap
     */
    template <typename TMapID, typename TKey, typename TValue>
    class CompiledMap <TMapID, TKey, TValue, SINGLE> {
        private:
            /// @brief Deleted constructor as this class is a static class.
            CompiledMap() = delete;
            /// @brief Deleted destructor as this class is a static class.
            ~CompiledMap() = delete;
            /// @brief the data variable where the data is stored for this map key.
            static std::shared_ptr<TValue> m_data;
            
        public:
            /**
             * @brief Stores the passed value in this map.
             *
             * @param data a pointer to the data to be stored (the map takes ownership)
             */
            static void set(TValue* data) {
                m_data = std::shared_ptr<TValue>(data);
            };
        
            /**
             * @brief Gets the value that was previously stored.
             *
             * @return a shared_ptr to the data that was previously stored
             *
             * @throws NoDataException if there is no data that was previously stored
             */
            static std::shared_ptr<TKey> get() {
                //If the pointer is not a nullptr
                if(*m_data) {
                    return m_data;
                }
                else {
                    throw NoDataException();
                }
            }
    };
    
    /**
     * @brief
     *  This map acts as a ring buffer of data, with the ability to get both the most recent data element as well
     *  as the previous n elements.
     *
     * @details
     *  This map acts as a ring buffer. When data is stored, it will remove the oldest element that is stored up to
     *  the capacity of the queue. The list of previous elements are accessable using the get(int) method.
     *
     * @see CompiledMap
     */
    template <typename TMapID, typename TKey, typename TValue>
    class CompiledMap <TMapID, TKey, TValue, QUEUE> {
        private:
            /// @brief Deleted constructor as this class is a static class
            CompiledMap() = delete;
            /// @brief Deleted destructor as this class is a static class
            ~CompiledMap() = delete;
            /// @brief the number of previous elements that we store
            static int m_capacity;
            /// @brief the queue where we store mapped elements
            static std::deque<std::shared_ptr<TValue>> m_data;
            
        public:
            /**
             * @brief increases the minimum capacity of stored objects to the passed integer.
             *
             * @param num the minimum number of previous elements to store
             */
            static void minCapacity(int num) {
                for(;m_capacity < num; ++m_capacity) {
                    m_data.emplace_back(nullptr);
                }
            }
        
            /**
             * @brief Stores the passed pointer in our storage, removing the oldest element for this key
             *
             * @param data a pointer to the data to store (the map takes ownership of it)
             */
            static void set(TValue* data) {
                m_data.pop_back();
                m_data.push_front(std::shared_ptr<TValue>(data));
            }
        
            /**
             * @brief Gets the most recent value that was previously stored.
             *
             * @return a shared_ptr to the most recent data that was stored
             *
             * @throws NoDataException if there is no data that was previously stored
             */
            static std::shared_ptr<TValue> get() {
                if(!m_data.empty() && m_data.front() != nullptr) {
                    return m_data.front();
                }
                else {
                    throw NoDataException();
                }
            };
        
            /**
             * @brief Gets the last n elements in the map for this key
             *
             * @return a vector with the last n elements that have been stored
             *
             * @attention make sure that minCapacity has been called with at least this length or a segfault will occur
             */
            static std::shared_ptr<std::vector<std::shared_ptr<const TValue>>> get(int length) {
                return std::shared_ptr<std::vector<std::shared_ptr<const TValue>>>(new std::vector<std::shared_ptr<const TValue>>(std::begin(m_data), std::begin(m_data) + length));
            };
    };
    
    
    /**
     * @brief
     *  This map acts as a map of lists, adding every stored element to the list of the set type.
     *
     * @details
     *  This map acts as a map of lists. When data is stored, add that element to the list of all items that are stored
     *
     * @see CompiledMap
     */
    template <typename TMapID, typename TKey, typename TValue>
    class CompiledMap <TMapID, TKey, TValue, LIST> {
        private:
            /// @brief Deleted constructor as this class is a static class
            CompiledMap() = delete;
            /// @brief Deleted destructor as this class is a static class
            ~CompiledMap() = delete;
            /// @brief the vector where we store the elements
            static std::vector<std::shared_ptr<TValue>> m_data;
            
        public:
            /**
             * @brief Stores the passed pointer in our storage, adding it to our collection of elements
             *
             * @param data a pointer to the data to store (the map takes ownership of it)
             */
            static void set(TValue* data) {
                m_data.push_back(std::shared_ptr<TValue>(data));
            }
            
            /**
             * @brief Gets the value that was previously stored.
             *
             * @return a vector of values that were previously stored
             *
             * @throws NoDataException if there is no data that was previously stored
             */
            static std::vector<std::shared_ptr<TValue>>& get() {
                return m_data;
            };
    };
    
    
    //Initialize the capacity of a QUEUE map
    template <typename TMapID, typename TKey, typename TValue>
    int CompiledMap<TMapID, TKey, TValue, QUEUE>::m_capacity = 1;
    
    //Initialize the deque object of a QUEUE map
    template <typename TMapID, typename TKey, typename TValue>
    std::deque<std::shared_ptr<TValue>> CompiledMap<TMapID, TKey, TValue, QUEUE>::m_data = std::deque<std::shared_ptr<TValue>>(1);
    
    //Initialize the vector of a LIST map
    template <typename TMapID, typename TKey, typename TValue>
    std::vector<std::shared_ptr<TValue>> CompiledMap<TMapID, TKey, TValue, LIST>::m_data = std::vector<std::shared_ptr<TValue>>();
}
}
}

#endif
