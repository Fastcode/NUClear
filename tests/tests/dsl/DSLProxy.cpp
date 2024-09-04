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

#include "test_util/TestBase.hpp"
#include "test_util/common.hpp"

struct CustomMessage1 {};
struct CustomMessage2 {
    CustomMessage2(int value) : value(value) {}
    int value;
};

namespace NUClear {
namespace dsl {
    namespace operation {
        template <>
        struct DSLProxy<CustomMessage1>
            : NUClear::dsl::operation::TypeBind<CustomMessage1>
            , NUClear::dsl::operation::CacheGet<CustomMessage2>
            , NUClear::dsl::word::Single {};
    }  // namespace operation
}  // namespace dsl
}  // namespace NUClear


class TestReactor : public test_util::TestBase<TestReactor> {
public:
    TestReactor(std::unique_ptr<NUClear::Environment> environment) : TestBase(std::move(environment)) {

        on<CustomMessage1>().then([this](const CustomMessage2& d) {
            events.push_back("CustomMessage1 Triggered with " + std::to_string(d.value));
        });

        on<Startup>().then([this]() {
            // Emit a double we can get
            events.push_back("Emitting CustomMessage2");
            emit(std::make_unique<CustomMessage2>(123456));

            // Emit a custom message 1 to trigger the reaction
            events.push_back("Emitting CustomMessage1");
            emit(std::make_unique<CustomMessage1>());
        });
    }

    /// Events that occur during the test
    std::vector<std::string> events;
};


TEST_CASE("Testing that the DSL proxy works as expected for binding unmodifyable types", "[api][dsl][proxy]") {

    NUClear::Configuration config;
    config.default_pool_concurrency = 1;
    NUClear::PowerPlant plant(config);
    test_util::add_tracing(plant);
    const auto& reactor = plant.install<TestReactor>();
    plant.start();

    const std::vector<std::string> expected = {
        "Emitting CustomMessage2",
        "Emitting CustomMessage1",
        "CustomMessage1 Triggered with 123456",
    };

    // Make an info print the diff in an easy to read way if we fail
    INFO(test_util::diff_string(expected, reactor.events));

    // Check the events fired in order and only those events
    REQUIRE(reactor.events == expected);
}
