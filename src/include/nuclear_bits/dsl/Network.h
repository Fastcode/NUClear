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

#ifndef NUCLEAR_DSL_NETWORK_H
#define NUCLEAR_DSL_NETWORK_H

namespace NUClear {
    namespace dsl {
        
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

#endif
