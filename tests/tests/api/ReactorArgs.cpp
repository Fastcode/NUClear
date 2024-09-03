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

class TestReactorNoArgs : public NUClear::Reactor {
public:
    TestReactorNoArgs(std::unique_ptr<NUClear::Environment> environment) : NUClear::Reactor(std::move(environment)) {}

    std::string s;
    bool b{false};
    uint32_t i{0};
};
class TestReactorArgs : public NUClear::Reactor {
public:
    TestReactorArgs(std::unique_ptr<NUClear::Environment> environment, std::string s, const bool& b, const uint32_t& i)
        : NUClear::Reactor(std::move(environment)), s(std::move(s)), b(b), i(i) {}

    std::string s;
    bool b{false};
    uint32_t i{0};
};


TEST_CASE("Testing Reactor installation arguments", "[api][reactorargs]") {
    NUClear::Configuration config;
    config.default_pool_concurrency = 1;
    NUClear::PowerPlant plant(config);
    const TestReactorArgs& r1   = plant.install<TestReactorArgs>("Hello NUClear", true, 0x00E298A2);
    const TestReactorNoArgs& r2 = plant.install<TestReactorNoArgs>();

    REQUIRE(r1.s == "Hello NUClear");
    REQUIRE(r1.b);
    REQUIRE(r1.i == 0x00E298A2);
    REQUIRE(r2.s.empty());
    REQUIRE(!r2.b);
    REQUIRE(r2.i == 0);
}
