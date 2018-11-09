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

// Anonymous namespace to keep everything file local
namespace {

int v1          = 0;
int v2          = 0;
int v3          = 0;
int stored_a    = 0;
double stored_c = 0;
double stored_d = 0;
std::string stored_b;

template <typename T>
struct EmitTester1 {
    static inline void emit(NUClear::PowerPlant& /*unused*/, std::shared_ptr<T> p, int a, std::string b) {
        v1       = *p;
        stored_a = a;
        stored_b = std::move(b);
    }

    static inline void emit(NUClear::PowerPlant& /*unused*/, std::shared_ptr<T> p, double c) {
        v2       = *p;
        stored_c = c;
    }
};

template <typename T>
struct EmitTester2 {
    static inline void emit(NUClear::PowerPlant& /*unused*/, std::shared_ptr<T> p, double d) {
        v3       = *p;
        stored_d = d;
    }
};

class TestReactor : public NUClear::Reactor {
public:
    TestReactor(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {

        // Make some things to emit
        auto t1 = std::make_unique<int>(8);
        auto t2 = std::make_unique<int>(10);
        auto t3 = std::make_unique<int>(52);
        auto t4 = std::make_unique<int>(100);

        // Test using the second overload
        emit<EmitTester1>(t1, 7.2);
        REQUIRE(v1 == 0);
        REQUIRE(v2 == 8);
        v2 = 0;
        REQUIRE(v3 == 0);
        REQUIRE(stored_a == 0);
        REQUIRE(stored_b == "");
        REQUIRE(stored_c == 7.2);
        stored_c = 0;
        REQUIRE(stored_d == 0);

        // Test using the first overload
        emit<EmitTester1>(t2, 1337, "This is text");
        REQUIRE(v1 == 10);
        v1 = 0;
        REQUIRE(v2 == 0);
        REQUIRE(v3 == 0);
        REQUIRE(stored_a == 1337);
        stored_a = 0;
        REQUIRE(stored_b == "This is text");
        stored_b = "";
        REQUIRE(stored_c == 0);
        REQUIRE(stored_d == 0);

        // Test multiple functions
        emit<EmitTester1, EmitTester2>(t3, 15, 8.3);
        REQUIRE(v1 == 0);
        REQUIRE(v2 == 52);
        v2 = 0;
        REQUIRE(v3 == 52);
        v3 = 0;
        REQUIRE(stored_a == 0);
        REQUIRE(stored_b == "");
        REQUIRE(stored_c == 15);
        stored_c = 0;
        REQUIRE(stored_d == 8.3);
        stored_d = 0;

        // Test even more multiple functions
        emit<EmitTester1, EmitTester2, EmitTester1>(t4, 2, "Hello World", 9.2, 5);
        REQUIRE(v1 == 100);
        REQUIRE(v2 == 100);
        REQUIRE(v3 == 100);
        REQUIRE(stored_a == 2);
        REQUIRE(stored_b == "Hello World");
        REQUIRE(stored_c == 5);
        REQUIRE(stored_d == 9.2);

        on<Startup>().then([this] { powerplant.shutdown(); });
    }
};
}  // namespace

TEST_CASE("Testing emit function fusion", "[api][emit][fusion]") {
    NUClear::PowerPlant::Configuration config;
    config.thread_count = 1;
    NUClear::PowerPlant plant(config);
    plant.install<TestReactor>();

    plant.start();
}
