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

#include <google/protobuf/message.h>
#include "NUClear/PowerPlant.h"
#include "NUClear/Networking/MurmurHash3.h"

namespace NUClear {
    namespace Networking {
        
        namespace {
            template <typename TType, bool IsProtoBuf = std::is_base_of<google::protobuf::Message, TType>::value>
            struct DefaultSerialization;
            
            template <typename TType>
            struct DefaultSerialization<TType, true> {
                
                static Hash hash() {
                    // We base the hash on the name of the protocol buffer
                    return murmurHash3(TType::New().GetTypeName().c_str, TType::New().GetTypeName().size());
                }
                
                static TType* deserialize(const std::string data) {
                    TType* buff = new TType();
                    buff->ParseFromString(data);
                    return buff;
                }
                
                static std::string serialize(TType* data) {
                    return data->SerializeAsString();
                }
            };
            
            template <typename TType>
            struct DefaultSerialization<TType, false> {
                
                static Hash hash() {
                    // We base the hash on the mangled name of the type
                    return murmurHash3(typeid(TType).name(), strlen(typeid(TType).name()));
                }
            
                static TType* deserialize(const std::string data) {
                    
                    TType* object = new TType();
                    memcpy(object, data.data(), sizeof(TType));
                    return object;
                }
                
                static std::string serialize(const TType* data) {
                    
                    const char* bytes = reinterpret_cast<const char*>(data);
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
            
            static TType* deserialize(const std::string data) {
                return DefaultSerialization<TType>::deserialize(data);
            }
            
            static std::string serialize(TType* data) {
                return DefaultSerialization<TType>::serialize(data);
            }
        };
    }
}
