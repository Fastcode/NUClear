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
std::vector<std::string> events;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

struct BindExtensionTest1 {
    template <typename DSL>
    static int bind(const std::shared_ptr<NUClear::threading::Reaction>& /*unused*/, const int& v1, const bool& v2) {
        events.push_back("Bind1 with " + std::to_string(v1) + " and " + (v2 ? "true" : "false") + " called");
        return 5;
    }
};

struct BindExtensionTest2 {
    template <typename DSL>
    static bool bind(const std::shared_ptr<NUClear::threading::Reaction>& /*reaction*/,
                     const std::string& v1,
                     const std::chrono::nanoseconds& v2) {
        events.push_back("Bind2 with " + v1 + " and " + std::to_string(v2.count()) + " called");
        return true;
    }
};

struct BindExtensionTest3 {
    template <typename DSL>
    static std::string bind(const std::shared_ptr<NUClear::threading::Reaction>& /*reaction*/,
                            const int& v1,
                            const int& v2,
                            const std::chrono::nanoseconds& v3) {
        events.push_back("Bind3 with " + std::to_string(v1) + ", " + std::to_string(v2) + " and "
                         + std::to_string(v3.count()) + " called");
        return "return from Bind3";
    }
};

class TestReactor : public test_util::TestBase<TestReactor> {
public:
    TestReactor(std::unique_ptr<NUClear::Environment> environment) : TestBase(std::move(environment)) {
        int a  = 0;
        bool b = false;
        std::string c;

        // Bind all three functions to test fission
        std::tie(std::ignore, a, b, c) =
            on<BindExtensionTest1, BindExtensionTest2, BindExtensionTest3>(5,
                                                                           false,
                                                                           "Hello",
                                                                           std::chrono::seconds(2),
                                                                           9,
                                                                           10,
                                                                           std::chrono::seconds(11))
                .then([] {});

        events.push_back("Bind1 returned " + std::to_string(a));
        events.push_back(std::string("Bind2 returned ") + (b ? "true" : "false"));
        events.push_back("Bind3 returned " + c);
    }
};


TEST_CASE("Testing distributing arguments to multiple bind functions (NUClear Fission)", "[api][dsl][fission]") {

    NUClear::Configuration config;
    config.default_pool_concurrency = 1;
    NUClear::PowerPlant plant(config);
    test_util::add_tracing(plant);
    plant.install<TestReactor>();
    plant.start();

    const std::vector<std::string> expected = {
        "Bind1 with 5 and false called",
        "Bind2 with Hello and 2000000000 called",
        "Bind3 with 9, 10 and 11000000000 called",
        "Bind1 returned 5",
        "Bind2 returned true",
        "Bind3 returned return from Bind3",
    };

    // Make an info print the diff in an easy to read way if we fail
    INFO(test_util::diff_string(expected, events));

    // Check the events fired in order and only those events
    REQUIRE(events == expected);
}
