/*
 * MIT License
 *
 * Copyright (c) 2015 NUClear Contributors
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
#include <utility>

#include "test_util/TestBase.hpp"
#include "test_util/common.hpp"

/// Events that occur during the test
std::vector<std::string> events;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables,-warnings-as-errors)

template <typename T>
struct E1 {
    static void emit(const NUClear::PowerPlant& /*powerplant*/,
                     std::shared_ptr<T> p,
                     const int& a,
                     const std::string& b) {
        events.push_back("E1a " + *p + " " + std::to_string(a) + " " + b);
    }

    static void emit(const NUClear::PowerPlant& /*powerplant*/, std::shared_ptr<T> p, const std::string& c) {
        events.push_back("E1b " + *p + " " + c);
    }
};

template <typename T>
struct E2 {
    static void emit(const NUClear::PowerPlant& /*powerplant*/, std::shared_ptr<T> p, const bool& d) {
        events.push_back("E2a " + *p + " " + (d ? "true" : "false"));
    }

    static void emit(const NUClear::PowerPlant& /*powerplant*/,
                     std::shared_ptr<T> p,
                     const int& e,
                     const std::string& f) {
        events.push_back("E2b " + *p + " " + std::to_string(e) + " " + f);
    }
};

class TestReactor : public test_util::TestBase<TestReactor> {
public:
    TestReactor(std::unique_ptr<NUClear::Environment> environment) : TestBase(std::move(environment)) {

        // Emit some messages
        emit<E1>(std::make_unique<std::string>("message1"), "test1");  // 1b
        events.push_back("End test 1");

        emit<E1>(std::make_unique<std::string>("message2"), 1337, "test2");  // 1a
        events.push_back("End test 2");

        emit<E1, E2>(std::make_unique<std::string>("message3"), 15, "test3", true);  // 1a, 2a
        events.push_back("End test 3");

        emit<E1, E2, E1>(std::make_unique<std::string>("message4"), 2, "Hello World", false, "test4");  // 1a, 2a, 1b
        events.push_back("End test 4");

        emit<E1, E2>(std::make_unique<std::string>("message5"), 5, "test5a", 10, "test5b");  // 1a, 2b
        events.push_back("End test 5");
    }
};


TEST_CASE("Testing emit function fusion", "[api][emit][fusion]") {

    NUClear::Configuration config;
    config.default_pool_concurrency = 1;
    NUClear::PowerPlant plant(config);
    plant.install<TestReactor>();

    const std::vector<std::string> expected = {
        "E1b message1 test1",
        "End test 1",
        "E1a message2 1337 test2",
        "End test 2",
        "E1a message3 15 test3",
        "E2a message3 true",
        "End test 3",
        "E1a message4 2 Hello World",
        "E2a message4 false",
        "E1b message4 test4",
        "End test 4",
        "E1a message5 5 test5a",
        "E2b message5 10 test5b",
        "End test 5",
    };

    // Make an info print the diff in an easy to read way if we fail
    INFO(test_util::diff_string(expected, events));

    // Check the events fired in order and only those events
    REQUIRE(events == expected);
}
