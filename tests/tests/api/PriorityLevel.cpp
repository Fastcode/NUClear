/*
 * MIT License
 *
 * Copyright (c) 2024 NUClear Contributors
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

#include "PriorityLevel.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_all.hpp>
#include <ostream>
#include <sstream>
#include <tuple>

SCENARIO("PriorityLevel smart enum values can be constructed and converted appropriately") {
    GIVEN("A PriorityLevel and a corresponding string representation") {
        const auto test = GENERATE(table<std::string, NUClear::PriorityLevel::Value>({
            std::make_tuple("IDLE", NUClear::PriorityLevel::IDLE),
            std::make_tuple("LOW", NUClear::PriorityLevel::LOW),
            std::make_tuple("NORMAL", NUClear::PriorityLevel::NORMAL),
            std::make_tuple("HIGH", NUClear::PriorityLevel::HIGH),
            std::make_tuple("REALTIME", NUClear::PriorityLevel::REALTIME),
        }));

        const auto& expected_str   = std::get<0>(test);
        const auto& expected_value = std::get<1>(test);

        WHEN("constructing a PriorityLevel from the Value") {
            const NUClear::PriorityLevel log_level(expected_value);

            THEN("it should be equal to the corresponding string representation") {
                REQUIRE(static_cast<std::string>(log_level) == expected_str);
            }
        }

        WHEN("constructing a PriorityLevel from the string") {
            const NUClear::PriorityLevel log_level(expected_str);

            THEN("it should be equal to the corresponding Value") {
                REQUIRE(log_level() == expected_value);
                REQUIRE(log_level == expected_value);
                REQUIRE(log_level == NUClear::PriorityLevel(expected_value));
            }
        }

        WHEN("constructing a PriorityLevel from the Value") {
            const NUClear::PriorityLevel log_level(expected_value);

            THEN("it should be equal to the corresponding string representation") {
                REQUIRE(static_cast<std::string>(log_level) == expected_str);
                REQUIRE(log_level == expected_str);
            }
        }

        WHEN("streaming the PriorityLevel to an ostream") {
            std::ostringstream os;
            os << NUClear::PriorityLevel(expected_value);

            THEN("the output should be the corresponding string representation") {
                REQUIRE(os.str() == expected_str);
            }
        }

        WHEN("converting the PriorityLevel to a string") {
            const std::string str = NUClear::PriorityLevel(expected_value);

            THEN("it should be equal to the corresponding string representation") {
                REQUIRE(str == expected_str);
            }
        }
    }
}

SCENARIO("PriorityLevel comparison operators work correctly") {
    GIVEN("Two PriorityLevel enum values") {
        const NUClear::PriorityLevel::Value v1 = GENERATE(NUClear::PriorityLevel::IDLE,
                                                          NUClear::PriorityLevel::LOW,
                                                          NUClear::PriorityLevel::NORMAL,
                                                          NUClear::PriorityLevel::HIGH,
                                                          NUClear::PriorityLevel::REALTIME);
        const NUClear::PriorityLevel::Value v2 = GENERATE(NUClear::PriorityLevel::IDLE,
                                                          NUClear::PriorityLevel::LOW,
                                                          NUClear::PriorityLevel::NORMAL,
                                                          NUClear::PriorityLevel::HIGH,
                                                          NUClear::PriorityLevel::REALTIME);

        WHEN("one smart enum value is constructed") {
            const NUClear::PriorityLevel ll1(v1);
            AND_WHEN("they are compared using ==") {
                THEN("the result should be correct") {
                    REQUIRE((ll1 == v2) == (v1 == v2));
                }
            }
            AND_WHEN("they are compared using !=") {
                THEN("the result should be correct") {
                    REQUIRE((ll1 != v2) == (v1 != v2));
                }
            }
            AND_WHEN("they are compared using <") {
                THEN("the result should be correct") {
                    REQUIRE((ll1 < v2) == (v1 < v2));
                }
            }
            AND_WHEN("they are compared using >") {
                THEN("the result should be correct") {
                    REQUIRE((ll1 > v2) == (v1 > v2));
                }
            }
            AND_WHEN("they are compared using <=") {
                THEN("the result should be correct") {
                    REQUIRE((ll1 <= v2) == (v1 <= v2));
                }
            }
            AND_WHEN("they are compared using >=") {
                THEN("the result should be correct") {
                    REQUIRE((ll1 >= v2) == (v1 >= v2));
                }
            }
        }

        WHEN("two smart enum values are constructed") {
            const NUClear::PriorityLevel ll1(v1);
            const NUClear::PriorityLevel ll2(v2);
            AND_WHEN("they are compared using ==") {
                THEN("the result should be correct") {
                    REQUIRE((ll1 == ll2) == (v1 == v2));
                }
            }
            AND_WHEN("they are compared using !=") {
                THEN("the result should be correct") {
                    REQUIRE((ll1 != ll2) == (v1 != v2));
                }
            }
            AND_WHEN("they are compared using <") {
                THEN("the result should be correct") {
                    REQUIRE((ll1 < ll2) == (v1 < v2));
                }
            }
            AND_WHEN("they are compared using >") {
                THEN("the result should be correct") {
                    REQUIRE((ll1 > ll2) == (v1 > v2));
                }
            }
            AND_WHEN("they are compared using <=") {
                THEN("the result should be correct") {
                    REQUIRE((ll1 <= ll2) == (v1 <= v2));
                }
            }
            AND_WHEN("they are compared using >=") {
                THEN("the result should be correct") {
                    REQUIRE((ll1 >= ll2) == (v1 >= v2));
                }
            }
        }
    }
}

SCENARIO("PriorityLevel can be used in switch statements") {
    GIVEN("A PriorityLevel") {
        auto test = GENERATE(table<std::string, NUClear::PriorityLevel::Value>({
            {"IDLE", NUClear::PriorityLevel::IDLE},
            {"LOW", NUClear::PriorityLevel::LOW},
            {"NORMAL", NUClear::PriorityLevel::NORMAL},
            {"HIGH", NUClear::PriorityLevel::HIGH},
            {"REALTIME", NUClear::PriorityLevel::REALTIME},
        }));

        const auto& str   = std::get<0>(test);
        const auto& value = std::get<1>(test);
        const NUClear::PriorityLevel log_level(value);

        WHEN("used in a switch statement") {
            std::string result;
            switch (log_level) {
                case NUClear::PriorityLevel::IDLE: result = "IDLE"; break;
                case NUClear::PriorityLevel::LOW: result = "LOW"; break;
                case NUClear::PriorityLevel::NORMAL: result = "NORMAL"; break;
                case NUClear::PriorityLevel::HIGH: result = "HIGH"; break;
                case NUClear::PriorityLevel::REALTIME: result = "REALTIME"; break;
                case NUClear::PriorityLevel::FATAL: result = "FATAL"; break;
                default: result = "UNKNOWN"; break;
            }

            THEN("the result should be the corresponding string representation") {
                REQUIRE(result == str);
            }
        }
    }
}
