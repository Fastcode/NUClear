#include "util/Sleeper.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

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
            // Sleep a few times to seed the sleep accuracy
            for (int i = 0; i < 100; ++i) {
                sleeper.sleep_for(std::chrono::milliseconds(1));
            }

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

}  // namespace util
}  // namespace NUClear
