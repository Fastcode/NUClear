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

namespace {

int value = 0;
std::vector<std::pair<int, int>> value_pairs;

struct DataType {
    int value;
    bool good;

    operator bool() const {
        return good;
    }
};
}  // namespace

namespace NUClear {
namespace dsl {
    namespace trait {
        template <>
        struct is_transient<DataType> : public std::true_type {};
    }  // namespace trait
}  // namespace dsl
}  // namespace NUClear

namespace {
struct SimpleMessage {
    SimpleMessage(int value) : value(value){};
    int value;
};

struct TransientTypeGetter : public NUClear::dsl::operation::TypeBind<int> {

    template <typename DSL>
    static inline DataType get(NUClear::threading::Reaction& /*unused*/) {

        // We say for this test that our data is valid if it is odd
        return DataType{value, value % 2 == 1};
    }
};

class TestReactor : public NUClear::Reactor {
public:
    TestReactor(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {

        on<TransientTypeGetter, Trigger<SimpleMessage>>().then(
            [](const DataType& d, const SimpleMessage& m) { value_pairs.push_back(std::make_pair(m.value, d.value)); });

        on<Startup>().then([this] {
            // Our data starts off as invalid
            value = 0;

            // This should not start a run as our data is invalid
            emit(std::make_unique<SimpleMessage>(10));

            // Change our value to 1, our transient data is now valid
            value = 1;

            // This should execute our function resulting in the pair 10,1
            emit(std::make_unique<int>(0));

            // This should make our transient data invalid again
            value = 2;

            // This should execute our function resulting in the pair 20,1
            emit(std::make_unique<SimpleMessage>(20));

            // This should update to a new good value
            value = 5;

            // This should execute our function resulting in the pair 30,5
            emit(std::make_unique<SimpleMessage>(30));

            // This should execute our function resulting in the pair 30,5
            emit(std::make_unique<int>(0));

            // Value is now bad again
            value = 10;

            // This should execute our function resulting in the pair 30,5
            emit(std::make_unique<int>(0));
            // TODO(trent): technically the thing that triggered this resulted in invalid data but used old data, do we
            // want to stop this?
            // TODO(trent): This would result in two states, invalid data, and non existant data

            // We are finished the test
            powerplant.shutdown();
        });
    }
};
}  // namespace

TEST_CASE("Testing whether getters that return transient data can cache between calls", "[api][transient]") {

    NUClear::PowerPlant::Configuration config;
    config.thread_count = 1;
    NUClear::PowerPlant plant(config);
    plant.install<TestReactor>();

    plant.start();

    // Now we validate the list (which may be in a different order due to NUClear scheduling)
    std::sort(std::begin(value_pairs), std::end(value_pairs));

    // Check that it was all as expected
    REQUIRE(value_pairs.size() == 5);
    REQUIRE(value_pairs[0] == std::make_pair(10, 1));
    REQUIRE(value_pairs[1] == std::make_pair(20, 1));
    REQUIRE(value_pairs[2] == std::make_pair(30, 5));
    REQUIRE(value_pairs[3] == std::make_pair(30, 5));
    REQUIRE(value_pairs[4] == std::make_pair(30, 5));
}
