/*
 * MIT License
 *
 * Copyright (c) 2013 NUClear Contributors
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

#include <catch2/catch_test_macros.hpp>
#include <nuclear>
#include <sstream>

#include "test_util/TestBase.hpp"

namespace {

// Events that occur during the test
std::vector<std::string> events;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

class TestReactor : public test_util::TestBase<TestReactor> {
private:
    using CommandLineArguments = NUClear::message::CommandLineArguments;

public:
    TestReactor(std::unique_ptr<NUClear::Environment> environment) : TestBase(std::move(environment)) {

        on<Trigger<CommandLineArguments>>().then([](const CommandLineArguments& args) {
            std::stringstream output;
            for (const auto& arg : args) {
                output << arg << " ";
            }
            events.push_back("CommandLineArguments: " + output.str());
        });
    }
};
}  // namespace

TEST_CASE("Testing the Command Line argument capturing", "[api][command_line_arguments]") {
    const int argc     = 2;
    const char* argv[] = {"Hello", "World"};  // NOLINT(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
    NUClear::Configuration config;
    config.thread_count = 1;
    NUClear::PowerPlant plant(config, argc, argv);
    plant.install<TestReactor>();
    plant.start();

    const std::vector<std::string> expected = {"CommandLineArguments: Hello World "};

    // Make an info print the diff in an easy to read way if we fail
    INFO(test_util::diff_string(expected, events));

    // Check the events fired in order and only those events
    REQUIRE(events == expected);
}
