/*
 * Copyright (C) 2013      Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
 *               2014-2017 Trent Houliston <trent@houliston.me>
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

#include <catch.hpp>
#include <utility>

#include <nuclear>

namespace {
struct BindExtensionTest1 {

    static int val1;
    static double val2;

    template <typename DSL>
    static inline int bind(const std::shared_ptr<NUClear::threading::Reaction>& /*unused*/, int v1, double v2) {

        val1 = v1;
        val2 = v2;

        return 5;
    }
};

int BindExtensionTest1::val1    = 0;    // NOLINT
double BindExtensionTest1::val2 = 0.0;  // NOLINT

struct BindExtensionTest2 {

    static std::string val1;
    static std::chrono::nanoseconds val2;

    template <typename DSL>
    static inline double bind(const std::shared_ptr<NUClear::threading::Reaction>& /*unused*/,
                              std::string v1,
                              std::chrono::nanoseconds v2) {

        val1 = std::move(v1);
        val2 = v2;

        return 7.2;
    }
};

std::string BindExtensionTest2::val1              = "";                           // NOLINT
std::chrono::nanoseconds BindExtensionTest2::val2 = std::chrono::nanoseconds(0);  // NOLINT

struct BindExtensionTest3 {

    static int val1;
    static int val2;
    static int val3;

    template <typename DSL>
    static inline NUClear::threading::ReactionHandle
    bind(const std::shared_ptr<NUClear::threading::Reaction>& /*unused*/, int v1, int v2, int v3) {

        val1 = v1;
        val2 = v2;
        val3 = v3;

        return NUClear::threading::ReactionHandle(nullptr);
    }
};

int BindExtensionTest3::val1 = 0;  // NOLINT
int BindExtensionTest3::val2 = 0;  // NOLINT
int BindExtensionTest3::val3 = 0;  // NOLINT

struct ShutdownFlag {};

class TestReactor : public NUClear::Reactor {
public:
    TestReactor(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {
        int a;
        double b;

        // Run all three of our extension tests
        std::tie(std::ignore, a, b, std::ignore) = on<BindExtensionTest1, BindExtensionTest2, BindExtensionTest3>(
                                                       5, 7.9, "Hello", std::chrono::seconds(2), 9, 10, 11)
                                                       .then([] {});

        // Check the returns from the bind
        REQUIRE(a == 5);
        REQUIRE(b == 7.2);

        REQUIRE(BindExtensionTest1::val1 == 5);
        REQUIRE(BindExtensionTest1::val2 == 7.9);

        REQUIRE(BindExtensionTest2::val1 == "Hello");
        REQUIRE(BindExtensionTest2::val2.count() == std::chrono::nanoseconds(2 * std::nano::den).count());

        REQUIRE(BindExtensionTest3::val1 == 9);
        REQUIRE(BindExtensionTest3::val2 == 10);
        REQUIRE(BindExtensionTest3::val3 == 11);

        // Run a test when there are blanks in the list before filled elements
        on<Trigger<int>, BindExtensionTest1>(2, 3.3).then([] {});

        on<Trigger<ShutdownFlag>>().then([this] {
            // We are finished the test
            powerplant.shutdown();
        });
    }
};
}  // namespace

TEST_CASE("Testing distributing arguments to multiple bind functions (NUClear Fission)", "[api][dsl][fission]") {

    NUClear::PowerPlant::Configuration config;
    config.thread_count = 1;
    NUClear::PowerPlant plant(config);
    plant.install<TestReactor>();

    plant.emit(std::make_unique<ShutdownFlag>());

    plant.start();
}
