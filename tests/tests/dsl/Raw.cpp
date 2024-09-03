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

class TestReactor : public test_util::TestBase<TestReactor> {
public:
    struct TypeA {
        TypeA(int x) : x(x) {}
        int x;
    };

    struct TypeB {
        TypeB(int x) : x(x) {}
        int x;
    };

    TestReactor(std::unique_ptr<NUClear::Environment> environment) : TestBase(std::move(environment)) {

        // Trigger on TypeA and store the result
        on<Trigger<TypeA>, MainThread>().then([this](const std::shared_ptr<const TypeA>& a) {
            stored.push_back(a);

            // Wait until we have 10 elements
            if (stored.size() == 10) {
                emit(std::make_unique<TypeB>(0));
            }
            else {
                emit(std::make_unique<TypeA>(a->x + 1));
            }
        });

        on<Trigger<TypeB>, MainThread>().then([this](const TypeB&) {
            // Make sure that our type a list has numbers 0 to 9

            REQUIRE(stored.size() == 10);

            for (int i = 0; i < int(stored.size()); ++i) {
                REQUIRE(stored[i]->x == i);
            }
        });

        on<Startup>().then([this] { emit(std::make_unique<TypeA>(0)); });
    }

    std::vector<std::shared_ptr<const TypeA>> stored;
};


TEST_CASE("Testing the raw type conversions work properly", "[api][raw]") {

    NUClear::Configuration config;
    config.default_pool_concurrency = 1;
    NUClear::PowerPlant plant(config);
    test_util::add_tracing(plant);
    plant.install<TestReactor>();

    plant.start();
}
