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

#ifndef TEST_UTIL_COMMON_HPP
#define TEST_UTIL_COMMON_HPP

#include <nuclear>

#include "executable_path.hpp"

namespace test_util {

/**
 * Adds tracing functionality to the given NUClear::PowerPlant.
 *
 * This function installs the NUClear::extension::TraceController extension into the power plant
 * and emits a NUClear::message::BeginTrace message to start tracing.
 *
 * The trace file will be written to the same location as the test binary with a .trace extension.
 *
 * @param plant The NUClear::PowerPlant to add tracing to.
 */
inline void add_tracing(NUClear::PowerPlant& plant) {
    auto test_binary_path = get_executable_path();
    plant.install<NUClear::extension::TraceController>();
    plant.emit<NUClear::dsl::word::emit::Inline>(
        std::make_unique<NUClear::message::BeginTrace>(test_binary_path + ".trace"));
}

}  // namespace test_util

#endif  // TEST_UTIL_COMMON_HPP
