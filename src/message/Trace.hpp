/*
 * MIT License
 *
 * Copyright (c) 2024 NUClear Contributors
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

#ifndef NUCLEAR_MESSAGE_TRACE_HPP
#define NUCLEAR_MESSAGE_TRACE_HPP

#include "../clock.hpp"

namespace NUClear {
namespace message {
    /**
     * This message will start recording a trace of the system to the specified file.
     */
    struct BeginTrace {
        BeginTrace(std::string file = "trace.trace", const bool& logs = true) : file(std::move(file)), logs(logs) {}
        /// The file to write the trace to
        std::string file;
        /// If log messages should be included in the trace
        bool logs;
    };

    /**
     * This message will stop recording the trace of the system.
     */
    struct EndTrace {};

}  // namespace message
}  // namespace NUClear

#endif  // NUCLEAR_MESSAGE_TRACE_HPP
