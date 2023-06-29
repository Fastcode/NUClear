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
struct BlankMessage {};

int i                = 0;      // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
bool emitted_message = false;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

class TestReactor : public NUClear::Reactor {
public:
    TestReactor(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {

        on<Always>().then([this] {
            // Run until it's 11 then emit the blank message
            if (i > 10) {
                if (!emitted_message) {
                    emitted_message = true;
                    emit(std::make_unique<BlankMessage>());
                }
            }
            else {
                ++i;
            }
        });

        on<Always, With<BlankMessage>>().then([this] {
            if (i == 11) {
                ++i;
                powerplant.shutdown();
            }
        });
    }
};
}  // namespace

TEST_CASE("Testing on<Always> functionality (permanent run)", "[api][always]") {

    NUClear::PowerPlant::Configuration config;
    config.thread_count = 1;
    NUClear::PowerPlant plant(config);

    // We are installing with an initial log level of debug
    plant.install<TestReactor, NUClear::DEBUG>();

    plant.emit(std::make_unique<int>(5));

    plant.start();

    REQUIRE(emitted_message);
    REQUIRE(i == 12);
}
