/*
 * MIT License
 *
 * Copyright (c) 2023 NUClear Contributors
 *
 * This file is part of the NUClear codebase.
 * See https://github.com/Fastcode/NUClear for further info.
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

#ifndef NUCLEAR_UTIL_NETWORK_IF_NUMBER_FROM_ADDRESS_HPP
#define NUCLEAR_UTIL_NETWORK_IF_NUMBER_FROM_ADDRESS_HPP

struct sockaddr_in6;

namespace NUClear {
namespace util {
    namespace network {

        /**
         * Gets the index of the interface that the given ipv6 address is on.
         *
         * @param ipv6 The ipv6 address to check
         *
         * @return The index of the interface that the address is on or 0 if the ipv6 address is the any address
         */
        unsigned int if_number_from_address(const sockaddr_in6& ipv6);

    }  // namespace network
}  // namespace util
}  // namespace NUClear

#endif  // NUCLEAR_UTIL_NETWORK_IF_NUMBER_FROM_ADDRESS_HPP
