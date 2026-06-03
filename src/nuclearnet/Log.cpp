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

#include "Log.hpp"

#include <cstdio>
#include <iostream>

namespace NUClear {
namespace network {

namespace {

    std::atomic<int> g_log_level{static_cast<int>(LogLevel::Off)};

    const char* level_name(LogLevel level) {
        switch (level) {
            case LogLevel::Error: return "error";
            case LogLevel::Warn: return "warn";
            case LogLevel::Info: return "info";
            case LogLevel::Debug: return "debug";
            case LogLevel::Trace: return "trace";
            default: return "off";
        }
    }

}  // namespace

    void set_log_level(LogLevel level) {
        g_log_level.store(static_cast<int>(level), std::memory_order_relaxed);
    }

    LogLevel get_log_level() {
        return static_cast<LogLevel>(g_log_level.load(std::memory_order_relaxed));
    }

    void log(LogLevel level, const char* component, const std::string& message) {
        if (!should_log(level)) {
            return;
        }
        std::cerr << "[NUClearNet:" << component << "] " << level_name(level) << " " << message << std::endl;
    }

    std::string hash_hex(uint64_t hash) {
        char buf[19];
        std::snprintf(buf, sizeof(buf), "0x%016llx", static_cast<unsigned long long>(hash));
        return buf;
    }

    std::string sock_str(const util::network::sock_t& address) {
        const auto addr = address.address();
        return addr.first + ":" + std::to_string(addr.second);
    }

    const char* handshake_str(HandshakeState state) {
        switch (state) {
            case HandshakeState::IDLE: return "IDLE";
            case HandshakeState::SYN_SENT: return "SYN_SENT";
            case HandshakeState::SYN_RECEIVED: return "SYN_RECEIVED";
            case HandshakeState::CONFIRMED: return "CONFIRMED";
        }
        return "UNKNOWN";
    }

}  // namespace network
}  // namespace NUClear
