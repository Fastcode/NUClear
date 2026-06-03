/*
 * MIT License
 *
 * Copyright (c) 2025 NUClear Contributors
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

#ifndef NUCLEAR_NETWORK_LOG_HPP
#define NUCLEAR_NETWORK_LOG_HPP

#include <atomic>
#include <cstdint>
#include <string>

#include "../util/network/sock_t.hpp"
#include "Discovery.hpp"

namespace NUClear {
namespace network {

    enum class LogLevel : int {
        Off   = 0,
        Error = 1,
        Warn  = 2,
        Info  = 3,
        Debug = 4,
        Trace = 5,
    };

    void set_log_level(LogLevel level);
    LogLevel get_log_level();

    inline bool should_log(LogLevel level) {
        return static_cast<int>(level) <= static_cast<int>(get_log_level());
    }

    void log(LogLevel level, const char* component, const std::string& message);

    std::string hash_hex(uint64_t hash);
    std::string sock_str(const util::network::sock_t& address);
    const char* handshake_str(HandshakeState state);

}  // namespace network
}  // namespace NUClear

#endif  // NUCLEAR_NETWORK_LOG_HPP
