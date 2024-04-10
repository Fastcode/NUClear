#define CATCH_CONFIG_MAIN
#include <catch.hpp>
#include <nuclear>

#include "test_util/TestBase.hpp"

namespace {

constexpr NUClear::clock::duration time_unit = std::chrono::milliseconds(10);

class TestReactor : public test_util::TestBase<TestReactor, 5000> {
public:
    // Start time of steady clock
    std::chrono::steady_clock::time_point steady_start_time = std::chrono::steady_clock::now();

    // Time travel action
    NUClear::message::TimeTravel::Action action = NUClear::message::TimeTravel::Action::RELATIVE;

    // Time adjustment
    NUClear::clock::duration adjustment = std::chrono::milliseconds(0);

    // Real-time factor
    double rtf = 1.0;

    // Expected results = {steady_delta_task_1, steady_delta_task_2}
    std::array<NUClear::clock::duration, 2> results = {std::chrono::milliseconds(0), std::chrono::milliseconds(0)};

    TestReactor(std::unique_ptr<NUClear::Environment> environment) : TestBase(std::move(environment), false) {

        on<Startup>().then([this] {
            steady_start_time = std::chrono::steady_clock::now();

            // Emit a chrono task to run at time 2
            auto chrono_task_1 = std::make_unique<NUClear::dsl::operation::ChronoTask>(
                [this](NUClear::clock::time_point&) {
                    results[0] = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()
                                                                                       - steady_start_time);
                    return false;
                },
                NUClear::clock::now() + 2 * time_unit,
                1);
            emit<Scope::DIRECT>(std::move(chrono_task_1));

            // Emit a chrono task to run at time 4, and shutdown
            auto chrono_task_2 = std::make_unique<NUClear::dsl::operation::ChronoTask>(
                [this](NUClear::clock::time_point&) {
                    results[1] = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()
                                                                                       - steady_start_time);
                    powerplant.shutdown();
                    return false;
                },
                NUClear::clock::now() + 4 * time_unit,
                1);
            emit<Scope::DIRECT>(std::move(chrono_task_2));

            // Time travel!
            emit<Scope::DIRECT>(std::make_unique<NUClear::message::TimeTravel>(adjustment, rtf, action));
        });
    }
};

}  // anonymous namespace


// TEST_CASE("Test time travel correctly changes the time for zero rtf", "[time_travel][chrono_controller]") {

// }

TEST_CASE("Test time travel correctly changes the time for non zero rtf", "[time_travel][chrono_controller]") {

    using Action = NUClear::message::TimeTravel::Action;

    const NUClear::Configuration config;
    auto plant    = std::make_shared<NUClear::PowerPlant>(config);
    auto& reactor = plant->install<TestReactor>();

    // Set the reactor fields to the values we want to test
    auto action        = GENERATE(Action::RELATIVE, Action::ABSOLUTE, Action::NEAREST);
    auto adjustment    = GENERATE(-2, -1, 0, 1, 2, 3, 4, 5);
    auto rtf           = GENERATE(0.5, 1.0, 2.0);
    reactor.action     = action;
    reactor.adjustment = adjustment * time_unit;
    reactor.rtf        = rtf;

    // Reset clock to zero
    NUClear::clock::set_clock(NUClear::clock::time_point());

    // Start the powerplant
    plant->start();

    // Expected results = {steady_delta_task_1, steady_delta_task_2}
    std::array<NUClear::clock::duration, 2> expected_results;
    switch (action) {
        case Action::RELATIVE:
            expected_results = {std::chrono::duration_cast<NUClear::clock::duration>((2 * time_unit) / rtf),
                                std::chrono::duration_cast<NUClear::clock::duration>((4 * time_unit) / rtf)};
            break;
        case Action::ABSOLUTE:
            expected_results = {
                std::chrono::duration_cast<NUClear::clock::duration>((std::max(0, 2 - adjustment) * time_unit) / rtf),
                std::chrono::duration_cast<NUClear::clock::duration>((std::max(0, 4 - adjustment) * time_unit) / rtf)};
            break;
        case Action::NEAREST:
            if (adjustment < 2) {
                expected_results = {
                    std::chrono::duration_cast<NUClear::clock::duration>((2 - adjustment) * time_unit / rtf),
                    std::chrono::duration_cast<NUClear::clock::duration>((4 - adjustment) * time_unit / rtf)};
            }
            else if (adjustment >= 2) {
                expected_results = {std::chrono::milliseconds(0),
                                    std::chrono::duration_cast<NUClear::clock::duration>((2 * time_unit) / rtf)};
            }
            break;
    }

    // Check the results
    INFO("action: " << static_cast<int>(action) << ", adjustment: " << adjustment << ", rtf: " << rtf);
    INFO("results: " << reactor.results[0].count() << ", " << reactor.results[1].count());
    INFO("expected: " << expected_results[0].count() << ", " << expected_results[1].count());
    REQUIRE(std::abs(reactor.results[0].count() - expected_results[0].count()) < time_unit.count());
    REQUIRE(std::abs(reactor.results[1].count() - expected_results[1].count()) < time_unit.count());
}
