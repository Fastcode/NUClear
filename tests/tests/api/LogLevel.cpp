#define CATCH_CONFIG_MAIN
#include "LogLevel.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_all.hpp>
#include <ostream>
#include <sstream>
#include <tuple>

SCENARIO("LogLevel smart enum values can be constructed and converted appropriately") {
    GIVEN("A LogLevel and a corresponding string representation") {
        const auto test = GENERATE(
            table<std::string, NUClear::LogLevel::Value>({std::make_tuple("TRACE", NUClear::LogLevel::TRACE),
                                                          std::make_tuple("DEBUG", NUClear::LogLevel::DEBUG),
                                                          std::make_tuple("INFO", NUClear::LogLevel::INFO),
                                                          std::make_tuple("WARN", NUClear::LogLevel::WARN),
                                                          std::make_tuple("ERROR", NUClear::LogLevel::ERROR),
                                                          std::make_tuple("FATAL", NUClear::LogLevel::FATAL)}));

        const auto& expected_str   = std::get<0>(test);
        const auto& expected_value = std::get<1>(test);

        WHEN("constructing a LogLevel from the Value") {
            const NUClear::LogLevel log_level(expected_value);

            THEN("it should be equal to the corresponding string representation") {
                REQUIRE(static_cast<std::string>(log_level) == expected_str);
            }
        }

        WHEN("constructing a LogLevel from the string") {
            const NUClear::LogLevel log_level(expected_str);

            THEN("it should be equal to the corresponding Value") {
                REQUIRE(log_level() == expected_value);
                REQUIRE(log_level == expected_value);
                REQUIRE(log_level == NUClear::LogLevel(expected_value));
            }
        }

        WHEN("constructing a LogLevel from the Value") {
            const NUClear::LogLevel log_level(expected_value);

            THEN("it should be equal to the corresponding string representation") {
                REQUIRE(static_cast<std::string>(log_level) == expected_str);
                REQUIRE(log_level == expected_str);
            }
        }

        WHEN("streaming the LogLevel to an ostream") {
            std::ostringstream os;
            os << NUClear::LogLevel(expected_value);

            THEN("the output should be the corresponding string representation") {
                REQUIRE(os.str() == expected_str);
            }
        }

        WHEN("converting the LogLevel to a string") {
            const std::string str = NUClear::LogLevel(expected_value);

            THEN("it should be equal to the corresponding string representation") {
                REQUIRE(str == expected_str);
            }
        }
    }
}

SCENARIO("LogLevel comparison operators work correctly") {
    GIVEN("Two LogLevel enum values") {
        const NUClear::LogLevel::Value v1 = GENERATE(NUClear::LogLevel::TRACE,
                                                     NUClear::LogLevel::DEBUG,
                                                     NUClear::LogLevel::INFO,
                                                     NUClear::LogLevel::WARN,
                                                     NUClear::LogLevel::ERROR,
                                                     NUClear::LogLevel::FATAL);
        const NUClear::LogLevel::Value v2 = GENERATE(NUClear::LogLevel::TRACE,
                                                     NUClear::LogLevel::DEBUG,
                                                     NUClear::LogLevel::INFO,
                                                     NUClear::LogLevel::WARN,
                                                     NUClear::LogLevel::ERROR,
                                                     NUClear::LogLevel::FATAL);

        WHEN("one smart enum value is constructed") {
            const NUClear::LogLevel ll1(v1);
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
            const NUClear::LogLevel ll1(v1);
            const NUClear::LogLevel ll2(v2);
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

SCENARIO("LogLevel can be used in switch statements") {
    GIVEN("A LogLevel") {
        auto test         = GENERATE(table<std::string, NUClear::LogLevel::Value>({{"TRACE", NUClear::LogLevel::TRACE},
                                                                                   {"DEBUG", NUClear::LogLevel::DEBUG},
                                                                                   {"INFO", NUClear::LogLevel::INFO},
                                                                                   {"WARN", NUClear::LogLevel::WARN},
                                                                                   {"ERROR", NUClear::LogLevel::ERROR},
                                                                                   {"FATAL", NUClear::LogLevel::FATAL}}));
        const auto& str   = std::get<0>(test);
        const auto& value = std::get<1>(test);
        const NUClear::LogLevel log_level(value);

        WHEN("used in a switch statement") {
            std::string result;
            switch (log_level) {
                case NUClear::LogLevel::TRACE: result = "TRACE"; break;
                case NUClear::LogLevel::DEBUG: result = "DEBUG"; break;
                case NUClear::LogLevel::INFO: result = "INFO"; break;
                case NUClear::LogLevel::WARN: result = "WARN"; break;
                case NUClear::LogLevel::ERROR: result = "ERROR"; break;
                case NUClear::LogLevel::FATAL: result = "FATAL"; break;
                default: result = "UNKNOWN"; break;
            }

            THEN("the result should be the corresponding string representation") {
                REQUIRE(result == str);
            }
        }
    }
}
