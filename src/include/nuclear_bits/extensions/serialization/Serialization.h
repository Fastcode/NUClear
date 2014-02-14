/*
 * Copyright (C) 2013 Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
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

#ifndef NUCLEAR_EXTENSIONS_SERIALIZATION_SERIALIZATION_H
#define NUCLEAR_EXTENSIONS_SERIALIZATION_SERIALIZATION_H

#include <cxxabi.h>
#include <type_traits>
#include <google/protobuf/message.h>

#include "nuclear_bits/PowerPlant.h"
#include "nuclear_bits/extensions/serialization/MurmurHash3.h"

namespace NUClear {
    namespace extensions {
        namespace serialization {
            
            /**
             * @brief This method returns the demangled name of the template type this is called with
             *
             * @details
             *  This uses the cxxabi in order to demangle the name of the type that is specified in TType.
             *
             * @tparam TType the type to demangle.
             *
             * @return the demangled name of this type
             */
            template <typename TType>
            const std::string demangled() {
                
                int status = -4; // some arbitrary value to eliminate the compiler warning
				std::unique_ptr<char, void(*)(void*)> res {
					abi::__cxa_demangle(typeid(TType).name(), nullptr, nullptr, &status),
					std::free
				};
                
                return std::string(status == 0 ? res.get() : typeid(TType).name());
            }
            
            //TODO find a way to use std::enable_if to pick protocol buffers or block copying
            
            namespace {
                template <typename TType, bool IsProtoBuf = std::is_base_of<google::protobuf::Message, TType>::value>
                struct DefaultSerialization;
                
                /**
                 * @brief This partial specialization is used for protocol buffers.
                 *
                 * @tparam TType the type we are serializing.
                 *
                 * @author Trent Houliston
                 */
                template <typename TType>
                struct DefaultSerialization<TType, true> {
                    
                    /**
                     * @brief Hashes the name of the protocol buffer for use as an identifier.
                     *
                     * @return A hash based on the name of the protocol buffer.
                     */
                    static const Hash hash() {
                        
                        // We have to construct an instance to call the reflection functions
                        TType type;
                        // We base the hash on the name of the protocol buffer
                        return murmurHash3(type.GetTypeName().c_str(), type.GetTypeName().size());
                    }
                    
                    /**
                     * @brief Deserializes the protocol buffer into an object.
                     *
                     * @param data the data to deserialize.
                     *
                     * @return a protocol buffer object.
                     */
                    static TType deserialize(const std::string& data) {
                        // Make a buffer
                        TType buff;
                        
                        // Deserialize it
                        buff.ParseFromString(data);
                        return buff;
                    }
                    
                    /**
                     * @brief Serializes a protocol buffer into binary.
                     *
                     * @param data the protocol buffer to serialize.
                     *
                     * @return an std::string containing the bytes to send.
                     */
                    static std::string serialize(const TType& data) {
                        return data.SerializeAsString();
                    }
                };
                
                /**
                 * This type handles the serialization of trivially copyable types.
                 *
                 * @tparam TType the type we are serializing.
                 *
                 * @author Trent Houliston
                 */
                template <typename TType>
                struct DefaultSerialization<TType, false> {
                    
                    // Assert that the type is trivially copyable (we can directly copy the bytes)
                    static_assert(std::is_trivial<TType>::value, "Only trivial classes can be serialized using the default serializer.");
                    
                    /**
                     * @brief Hashes the demangled type name of the type for use as an identifier.
                     *
                     * @return A hash based on the demangled type name.
                     */
                    static const Hash hash() {
                        // We base the hash on the demangled name of the type
                        std::string typeName = demangled<TType>();
                        return murmurHash3(typeName.c_str(), typeName.size());
                    }
                    
                    /**
                     * @brief Deserializes the data buffer into an object.
                     *
                     * @param data the data to deserialize.
                     *
                     * @return the object with its data filled.
                     */
                    static TType deserialize(const std::string data) {
                        
                        // Make an object
                        TType object;
                        
                        // Copy data into it
                        memcpy(&object, data.data(), sizeof(TType));
                        return object;
                    }
                    
                    /**
                     * @brief Serializes a trivially copyable object into binary.
                     *
                     * @param data the object to serialize.
                     *
                     * @return an std::string containing the bytes to send.
                     */
                    static std::string serialize(const TType& data) {
                        
                        const char* bytes = reinterpret_cast<const char*>(&data);
                        std::string result(bytes, sizeof(TType));
                        return result;
                    }
                };
            }
            
            /**
             * @brief This function is to be specialized to hash specific types.
             *
             * @tparam TType the type we are getting a hash for
             *
             * @return The hash for this type.
             */
            template <typename TType>
            Hash hash() {
                
                // If we have a protocol buffer use it otherwise do the default option
                return DefaultSerialization<TType>::hash();
            }
            
            /**
             * @brief This type is designed to be partially specialized to handle the serialization of specific objects.
             *
             * @tparam TType the type we are building a serializer for
             *
             * @author Trent Houliston
             */
            template <typename TType>
            struct Serializer {
                
                /**
                 * @brief Deserialize an object from a string.
                 *
                 * @param data the string we are converting an object from
                 *
                 * @return the object represented by the serialized form.
                 */
                static TType deserialize(const std::string data) {
                    return DefaultSerialization<TType>::deserialize(data);
                }
                
                /**
                 * @brief Serialize an object to a string.
                 *
                 * @param data the data we are to serialize.
                 *
                 * @return a string containing serialized bytes.
                 */
                static std::string serialize(const TType& data) {
                    return DefaultSerialization<TType>::serialize(std::forward<const TType&>(data));
                }
            };
        }
    }
}

#endif