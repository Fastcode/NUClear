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

#ifndef NUCLEAR_SERIALIZATION_H
#define NUCLEAR_SERIALIZATION_H

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
             *  This uses the cxxabi in order to demangle the name of the type that is specified in TType
             *
             * @tparam TType the type to demangle
             *
             * @returns the demangled name of this type
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
                
                template <typename TType>
                struct DefaultSerialization<TType, true> {
                    
                    static const Hash hash() {
                        TType type;
                        // We base the hash on the name of the protocol buffer
                        return murmurHash3(type.GetTypeName().c_str(), type.GetTypeName().size());
                    }
                    
                    static TType deserialize(const std::string& data) {
                        TType buff;
                        buff.ParseFromString(data);
                        return buff;
                    }
                    
                    static std::string serialize(const TType& data) {
                        return data.SerializeAsString();
                    }
                };
                
                template <typename TType>
                struct DefaultSerialization<TType, false> {
                    
                    // Assert that the type is trivially copyable (we can directly copy the bytes)
                    static_assert(std::is_trivial<TType>::value, "Only trivial classes can be serialized using the default serializer.");
                    
                    static const Hash hash() {
                        // We base the hash on the demangled name of the type
                        std::string typeName = demangled<TType>();
                        return murmurHash3(typeName.c_str(), typeName.size());
                    }
                    
                    static TType deserialize(const std::string data) {
                        
                        TType object;
                        memcpy(&object, data.data(), sizeof(TType));
                        return object;
                    }
                    
                    static std::string serialize(const TType& data) {
                        
                        const char* bytes = reinterpret_cast<const char*>(&data);
                        std::string result(bytes, sizeof(TType));
                        return result;
                    }
                };
            }
            
            template <typename TType>
            Hash hash() {
                
                // If we have a protocol buffer use it otherwise do the default option
                return DefaultSerialization<TType>::hash();
            }
            
            template <typename TType>
            struct Serializer {
                
                static TType deserialize(const std::string data) {
                    return DefaultSerialization<TType>::deserialize(data);
                }
                
                static std::string serialize(const TType& data) {
                    return DefaultSerialization<TType>::serialize(std::forward<const TType&>(data));
                }
            };
        }
    }
}

#endif