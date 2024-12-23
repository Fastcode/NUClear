#define CATCH_CONFIG_MAIN
#include "LogLevel.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <ostream>
#include <sstream>

SCENARIO("LogLevel can be constructed from Value") {
    GIVEN("A LogLevel constructed from a Value") {
        auto value = GENERATE(NUClear::LogLevel::TRACE,
                              NUClear::LogLevel::DEBUG,
                              NUClear::LogLevel::INFO,
                              NUClear::LogLevel::WARN,
                              NUClear::LogLevel::ERROR,
                              NUClear::LogLevel::FATAL);
        const NUClear::LogLevel log_level(value);

        WHEN("the value is retrieved") {
            auto retrieved_value = log_level();

            THEN("it should be equal to the original value") {
                REQUIRE(retrieved_value == value);
            }
        }
    }
}

SCENARIO("LogLevel can be constructed from a string") {
    GIVEN("A LogLevel constructed from a string") {
        const auto str = GENERATE("TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL");
        const NUClear::LogLevel log_level(str);

        WHEN("the value is retrieved") {
            auto value = log_level();

            THEN("it should be equal to the corresponding Value") {
                REQUIRE(log_level == str);
            }
        }
    }
}

SCENARIO("LogLevel can be converted to a string") {
    GIVEN("A LogLevel") {
        const auto value = GENERATE(NUClear::LogLevel::TRACE,
                                    NUClear::LogLevel::DEBUG,
                                    NUClear::LogLevel::INFO,
                                    NUClear::LogLevel::WARN,
                                    NUClear::LogLevel::ERROR,
                                    NUClear::LogLevel::FATAL);
        const NUClear::LogLevel log_level(value);

        WHEN("it is converted to a string") {
            std::string str = log_level;

            THEN("it should be equal to the corresponding string representation") {
                REQUIRE(str == log_level);
            }
        }
    }
}

SCENARIO("LogLevel can be streamed to an ostream") {
    GIVEN("A LogLevel") {
        const auto value = GENERATE(NUClear::LogLevel::TRACE,
                                    NUClear::LogLevel::DEBUG,
                                    NUClear::LogLevel::INFO,
                                    NUClear::LogLevel::WARN,
                                    NUClear::LogLevel::ERROR,
                                    NUClear::LogLevel::FATAL);
        const NUClear::LogLevel log_level(value);

        WHEN("it is streamed to an ostream") {
            std::ostringstream os;
            os << log_level;

            THEN("the output should be the corresponding string representation") {
                REQUIRE(os.str() == log_level);
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

        WHEN("two smart enum value is constructed") {
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
        const auto value = GENERATE(NUClear::LogLevel::TRACE,
                                    NUClear::LogLevel::DEBUG,
                                    NUClear::LogLevel::INFO,
                                    NUClear::LogLevel::WARN,
                                    NUClear::LogLevel::ERROR,
                                    NUClear::LogLevel::FATAL);
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
                REQUIRE(result == log_level);
            }
        }
    }
}
