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
#include <nuclear>

namespace NUClear {
namespace dsl {
    namespace operation {
        template <>
        struct DSLProxy<int>
            : public NUClear::dsl::operation::TypeBind<int>
            , public NUClear::dsl::operation::CacheGet<double>
            , public NUClear::dsl::word::Single {};
    }  // namespace operation
}  // namespace dsl
}  // namespace NUClear

namespace {

class TestReactor : public NUClear::Reactor {
public:
    TestReactor(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {

        on<int>().then([this](const double& d) {
            // The message we received should have test == 10
            REQUIRE(d == 4.4);

            // We are finished the test
            powerplant.shutdown();
        });

        on<Startup>().then([this]() {
            // Emit a double we can get
            emit(std::make_unique<double>(4.4));

            // Emit an integer to trigger the reaction
            emit(std::make_unique<int>());
        });
    }
};
}  // namespace

TEST_CASE("Testing that the DSL proxy works as expected for binding unmodifyable types", "[api][dsl][proxy]") {

    NUClear::PowerPlant::Configuration config;
    config.thread_count = 1;
    NUClear::PowerPlant plant(config);
    plant.install<TestReactor>();

    plant.start();
}
