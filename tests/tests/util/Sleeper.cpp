#include "util/Sleeper.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <thread>

#include "test_util/TimeUnit.hpp"
#include "util/update_current_thread_priority.hpp"

namespace NUClear {
namespace util {

    constexpr std::chrono::milliseconds max_error{2};

    SCENARIO("Sleeper provides precise sleep functionality", "[Sleeper]") {

        /// Set the priority to maximum to enable realtime to make more accurate sleeps
        update_current_thread_priority(1000);

        GIVEN("A Sleeper object") {
            // Sleep for a negative duration, 0, 10, and 20 milliseconds
            const int sleep_ms = GENERATE(-10, 0, 10, 20);

            Sleeper sleeper;
            WHEN("Sleeping for a specified duration") {
                auto sleep_duration    = std::chrono::milliseconds(sleep_ms);
                auto expected_duration = std::chrono::milliseconds(std::max(0, sleep_ms));

                auto start_time = std::chrono::steady_clock::now();

                sleeper.sleep_for(sleep_duration);

                auto end_time        = std::chrono::steady_clock::now();
                auto actual_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

                THEN("The sleep duration should be close to the specified duration") {
                    REQUIRE(actual_duration >= expected_duration);
                    REQUIRE(actual_duration <= expected_duration + max_error);
                }
            }

            WHEN("Sleeping until a specific time point") {
                auto sleep_duration    = std::chrono::milliseconds(sleep_ms);
                auto expected_duration = std::chrono::milliseconds(std::max(0, sleep_ms));

                auto start_time    = std::chrono::steady_clock::now();
                auto target_time   = start_time + sleep_duration;
                auto expected_time = start_time + expected_duration;

                sleeper.sleep_until(target_time);

                auto end_time = std::chrono::steady_clock::now();

                THEN("The wake-up time should be close to the target time") {
                    auto delta_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - expected_time);
                    CAPTURE(delta_ns);
                    REQUIRE(end_time >= expected_time);
                    REQUIRE(end_time <= expected_time + max_error);  // Allow for small discrepancies
                }
            }
        }
    }

    SCENARIO("Sleeper can be woken by another thread when it is sleeping") {

        /// Set the priority to maximum to enable realtime to make more accurate sleeps
        update_current_thread_priority(1000);

        GIVEN("A Sleeper object") {
            Sleeper sleeper;

            WHEN("The sleeper is sleeping and is woken by another thread") {
                std::thread wake_thread([&sleeper] {
                    std::this_thread::sleep_for(test_util::TimeUnit(2));
                    sleeper.wake();
                });

                auto start_time = std::chrono::steady_clock::now();
                sleeper.sleep_for(test_util::TimeUnit(20));
                auto end_time = std::chrono::steady_clock::now();

                wake_thread.join();

                THEN("The sleeper should wake up early") {
                    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
                    REQUIRE(duration < test_util::TimeUnit(3));
                }
            }
        }
    }

    SCENARIO("A Sleeper which is woken before it is slept on will not sleep") {

        /// Set the priority to maximum to enable realtime to make more accurate sleeps
        update_current_thread_priority(1000);

        GIVEN("A Sleeper object") {
            Sleeper sleeper;

            WHEN("The sleeper is woken before it is slept on") {
                sleeper.wake();
                auto start_time = std::chrono::steady_clock::now();
                sleeper.sleep_for(test_util::TimeUnit(10));
                auto end_time = std::chrono::steady_clock::now();

                THEN("The sleeper should not sleep") {
                    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
                    REQUIRE(duration < test_util::TimeUnit(1));
                }

                AND_WHEN("The sleeper sleeps again") {
                    auto start_time = std::chrono::steady_clock::now();
                    sleeper.sleep_for(test_util::TimeUnit(5));
                    auto end_time = std::chrono::steady_clock::now();

                    THEN("The sleeper should sleep normally") {
                        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
                        REQUIRE(duration >= test_util::TimeUnit(5));
                    }
                }
            }
        }
    }

}  // namespace util
}  // namespace NUClear
