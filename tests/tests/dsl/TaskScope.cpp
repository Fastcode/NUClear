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
#include "test_util/common.hpp"

/**
 * Holds information about a single step in the test
 */
struct StepData {
    int scope;                         ///< The scope that this step was run in
    bool next_inline;                  ///< If the next step was run inline
    std::array<bool, 3> scope_states;  ///< The state of the scopes during this step
};

namespace Catch {
template <>
struct StringMaker<std::map<int, StepData>> {
    static std::string convert(const std::map<int, StepData>& value) {
        std::stringstream event;
        for (const auto& items : value) {
            event << items.first << "(" << items.second.scope << "):";
            for (const auto& scope : items.second.scope_states) {
                event << (scope ? "t" : "f");
            }
            event << (items.second.next_inline ? " -> " : " -| ");
        }
        return event.str();
    }
};
}  // namespace Catch

class TestReactor : public test_util::TestBase<TestReactor> {
public:
    /**
     * Holds the accumulated data for the test
     */
    template <int Step>
    struct Data {
        std::map<int, StepData> steps;  ///< The steps that have been run
    };

    template <int Current, int ScopeID, typename Message>
    void process_step(const Message& d) {

        using NextData = Data<Current + 1>;

        // Get the scope state before the inline event
        auto next_inline            = std::make_unique<NextData>();
        next_inline->steps          = d.steps;
        next_inline->steps[Current] = {
            ScopeID,
            true,
            {TaskScope<Data<0>>::in_scope(), TaskScope<Data<1>>::in_scope(), TaskScope<Data<2>>::in_scope()},
        };
        emit<Scope::INLINE>(next_inline);

        // Get the scope state after the inline event
        auto next_normal            = std::make_unique<NextData>();
        next_normal->steps          = d.steps;
        next_normal->steps[Current] = {
            ScopeID,
            false,
            {TaskScope<Data<0>>::in_scope(), TaskScope<Data<1>>::in_scope(), TaskScope<Data<2>>::in_scope()}};
        emit(next_normal);
    }

    TestReactor(std::unique_ptr<NUClear::Environment> environment) : TestBase(std::move(environment)) {

        on<Trigger<Data<0>>>().then([this](const Data<0>& a) { process_step<0, -1>(a); });
        on<Trigger<Data<0>>, TaskScope<Data<0>>>().then([this](const Data<0>& m) { process_step<0, 0>(m); });
        on<Trigger<Data<0>>, TaskScope<Data<1>>>().then([this](const Data<0>& m) { process_step<0, 1>(m); });
        on<Trigger<Data<0>>, TaskScope<Data<2>>>().then([this](const Data<0>& m) { process_step<0, 2>(m); });

        on<Trigger<Data<1>>>().then([this](const Data<1>& m) { process_step<1, -1>(m); });
        on<Trigger<Data<1>>, TaskScope<Data<0>>>().then([this](const Data<1>& m) { process_step<1, 0>(m); });
        on<Trigger<Data<1>>, TaskScope<Data<1>>>().then([this](const Data<1>& m) { process_step<1, 1>(m); });
        on<Trigger<Data<1>>, TaskScope<Data<2>>>().then([this](const Data<1>& m) { process_step<1, 2>(m); });

        on<Trigger<Data<2>>>().then([this](const Data<2>& m) { process_step<2, -1>(m); });
        on<Trigger<Data<2>>, TaskScope<Data<0>>>().then([this](const Data<2>& m) { process_step<2, 0>(m); });
        on<Trigger<Data<2>>, TaskScope<Data<1>>>().then([this](const Data<2>& m) { process_step<2, 1>(m); });
        on<Trigger<Data<2>>, TaskScope<Data<2>>>().then([this](const Data<2>& m) { process_step<2, 2>(m); });

        // Store the results of the test
        on<Trigger<Data<3>>>().then([this](const Data<3>& m) { events.push_back(m.steps); });

        // Start the test
        on<Startup>().then([this] { emit(std::make_unique<Data<0>>()); });
    }

    /// A vector of events that have happened
    std::vector<std::map<int, StepData>> events;
};


TEST_CASE("Test that Trigger statements get the correct data", "[api][trigger]") {

    NUClear::Configuration config;
    config.default_pool_concurrency = 1;
    NUClear::PowerPlant plant(config);
    // test_util::add_tracing(plant);
    const auto& reactor = plant.install<TestReactor>();
    plant.start();

    CHECK(reactor.events.size() == 512);
    for (const auto& test : reactor.events) {
        CAPTURE(test);
        CHECK(test.size() == 3);
        for (const auto& step : test) {
            const auto& step_no = step.first;
            const auto& data    = step.second;
            CAPTURE(step_no);
            CAPTURE(data.scope);

            // Only the active scope (if there is one) should be true
            CHECK(data.scope_states[0] == (0 == data.scope));
            CHECK(data.scope_states[1] == (1 == data.scope));
            CHECK(data.scope_states[2] == (2 == data.scope));
        }
    }
}
