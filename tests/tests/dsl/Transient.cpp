/*
 * MIT License
 *
 * Copyright (c) 2023 NUClear Contributors
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

struct Message {
    Message(std::string msg) : msg(std::move(msg)) {}
    std::string msg;
};

struct TransientMessage {
    TransientMessage(std::string msg = "", bool valid = false) : msg(std::move(msg)), valid(valid) {}
    std::string msg;
    bool valid;

    operator bool() const {
        return valid;
    }
};

namespace NUClear {
namespace dsl {
    namespace trait {
        template <>
        struct is_transient<TransientMessage> : std::true_type {};
    }  // namespace trait
}  // namespace dsl
}  // namespace NUClear

struct TransientGetter : NUClear::dsl::operation::TypeBind<TransientMessage> {

    template <typename DSL>
    static TransientMessage get(NUClear::threading::ReactionTask& task) {

        // Get the real message and return it directly so transient can activate
        auto raw = NUClear::dsl::operation::CacheGet<TransientMessage>::get<DSL>(task);
        if (raw == nullptr) {
            return {};
        }
        return *raw;
    }
};

class TestReactor : public test_util::TestBase<TestReactor> {
public:
    TestReactor(std::unique_ptr<NUClear::Environment> environment) : TestBase(std::move(environment)) {

        on<Trigger<Message>, TransientGetter>().then([this](const Message& m, const TransientMessage& t) {  //
            events.push_back(m.msg + " : " + t.msg);
        });

        on<Trigger<Step<1>>, Priority::LOW>().then([this] {
            events.push_back("Emitting Message 1");
            emit(std::make_unique<Message>("S1"));
        });
        on<Trigger<Step<2>>, Priority::LOW>().then([this] {
            events.push_back("Emitting Transient 1");
            emit(std::make_unique<TransientMessage>("T1", true));
        });
        on<Trigger<Step<3>>, Priority::LOW>().then([this] {
            events.push_back("Emitting Message 2");
            emit(std::make_unique<Message>("S2"));
        });
        on<Trigger<Step<4>>, Priority::LOW>().then([this] {
            events.push_back("Emitting Invalid Transient 2");
            emit(std::make_unique<TransientMessage>("T2", false));
        });
        on<Trigger<Step<5>>, Priority::LOW>().then([this] {
            events.push_back("Emitting Message 3");
            emit(std::make_unique<Message>("S3"));
        });
        on<Trigger<Step<6>>, Priority::LOW>().then([this] {
            events.push_back("Emitting Transient 3");
            emit(std::make_unique<TransientMessage>("T3", true));
        });
        on<Trigger<Step<7>>, Priority::LOW>().then([this] {
            events.push_back("Emitting Transient 4");
            emit(std::make_unique<TransientMessage>("T4", true));
        });
        on<Trigger<Step<8>>, Priority::LOW>().then([this] {
            events.push_back("Emitting Invalid Transient 5");
            emit(std::make_unique<TransientMessage>("T5", false));
        });
        on<Trigger<Step<9>>, Priority::LOW>().then([this] {
            events.push_back("Emitting Message 4");
            emit(std::make_unique<Message>("S4"));
        });

        on<Startup>().then([this] {
            emit(std::make_unique<Step<1>>());
            emit(std::make_unique<Step<2>>());
            emit(std::make_unique<Step<3>>());
            emit(std::make_unique<Step<4>>());
            emit(std::make_unique<Step<5>>());
            emit(std::make_unique<Step<6>>());
            emit(std::make_unique<Step<7>>());
            emit(std::make_unique<Step<8>>());
            emit(std::make_unique<Step<9>>());
        });
    }

    /// Events that occur during the test
    std::vector<std::string> events;
};


TEST_CASE("Testing whether getters that return transient data can cache between calls", "[api][transient]") {
    NUClear::Configuration config;
    config.default_pool_concurrency = 1;
    NUClear::PowerPlant plant(config);
    test_util::add_tracing(plant);
    const auto& reactor = plant.install<TestReactor>();
    plant.start();

    const std::vector<std::string> expected = {
        "Emitting Message 1",
        "Emitting Transient 1",
        "S1 : T1",
        "Emitting Message 2",
        "S2 : T1",
        "Emitting Invalid Transient 2",
        "S2 : T1",
        "Emitting Message 3",
        "S3 : T1",
        "Emitting Transient 3",
        "S3 : T3",
        "Emitting Transient 4",
        "S3 : T4",
        "Emitting Invalid Transient 5",
        "S3 : T4",
        "Emitting Message 4",
        "S4 : T4",
    };

    // Make an info print the diff in an easy to read way if we fail
    INFO(test_util::diff_string(expected, reactor.events));

    // Check the events fired in order and only those events
    REQUIRE(reactor.events == expected);
}
