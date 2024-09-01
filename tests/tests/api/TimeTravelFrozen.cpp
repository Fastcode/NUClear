#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <future>
#include <nuclear>

#include "test_util/TestBase.hpp"
#include "test_util/common.hpp"
#include "util/precise_sleep.hpp"

constexpr std::chrono::milliseconds EVENT_1_TIME  = std::chrono::milliseconds(4);
constexpr std::chrono::milliseconds EVENT_2_TIME  = std::chrono::milliseconds(8);
constexpr std::chrono::milliseconds SHUTDOWN_TIME = std::chrono::milliseconds(12);


class TestReactor : public test_util::TestBase<TestReactor> {
public:
    struct WaitForShutdown {};

    TestReactor(std::unique_ptr<NUClear::Environment> environment) : TestBase(std::move(environment), false) {

        on<Startup>().then([this] {
            // Reset clock to zero
            NUClear::clock::set_clock(NUClear::clock::time_point(), 0.0);

            // Emit a chrono task to run at time EVENT_1_TIME
            emit<Scope::INLINE>(std::make_unique<NUClear::dsl::operation::ChronoTask>(
                [this](NUClear::clock::time_point&) {
                    add_event("Event 1");
                    return false;
                },
                NUClear::clock::time_point(EVENT_1_TIME),
                1));

            // Emit a chrono task to run at time EVENT_2_TIME
            emit<Scope::INLINE>(std::make_unique<NUClear::dsl::operation::ChronoTask>(
                [this](NUClear::clock::time_point&) {
                    add_event("Event 2");
                    return false;
                },
                NUClear::clock::time_point(EVENT_2_TIME),
                2));

            // Time travel
            emit<Scope::INLINE>(
                std::make_unique<NUClear::message::TimeTravel>(NUClear::clock::time_point(adjustment), rtf, action));

            // Shutdown after steady clock amount of time
            emit(std::make_unique<WaitForShutdown>());
        });

        on<Trigger<WaitForShutdown>>().then([this] {
            NUClear::util::precise_sleep(SHUTDOWN_TIME);
            add_event("Finished");
            powerplant.shutdown();
        });
    }

    // Time travel action
    NUClear::message::TimeTravel::Action action = NUClear::message::TimeTravel::Action::RELATIVE;

    // Time adjustment
    NUClear::clock::duration adjustment = std::chrono::milliseconds(0);

    // Real-time factor
    double rtf = 1.0;

    /// Events that occur during the test
    std::mutex event_mutex;
    std::vector<std::string> events;

private:
    void add_event(const std::string& event) {
        const std::lock_guard<std::mutex> lock(event_mutex);
        events.emplace_back(event);
    }
};


TEST_CASE("Test time travel correctly changes the time for non zero rtf", "[time_travel][chrono_controller]") {

    using Action = NUClear::message::TimeTravel::Action;

    const NUClear::Configuration config;
    NUClear::PowerPlant plant(config);
    test_util::add_tracing(plant);
    plant.install<NUClear::extension::ChronoController>();
    auto& reactor = plant.install<TestReactor>();

    // Set the reactor fields to the values we want to test
    const Action action      = GENERATE(Action::RELATIVE, Action::ABSOLUTE, Action::NEAREST);
    const int64_t adjustment = GENERATE(-4, -2, 0, 2, 4, 6, 8, 10);
    CAPTURE(action, adjustment);
    reactor.action     = action;
    reactor.adjustment = std::chrono::milliseconds(adjustment);
    reactor.rtf        = 0.0;

    // Start the powerplant
    plant.start();

    // Expected results
    std::vector<std::string> expected;
    switch (action) {
        case Action::RELATIVE: expected = {"Finished"}; break;
        case Action::ABSOLUTE:
            if (std::chrono::milliseconds(adjustment) < EVENT_1_TIME) {
                expected = {"Finished"};
            }
            else if (std::chrono::milliseconds(adjustment) < EVENT_2_TIME) {
                expected = {"Event 1", "Finished"};
            }
            else {
                expected = {"Event 1", "Event 2", "Finished"};
            }
            break;
        case Action::NEAREST:
            expected = std::chrono::milliseconds(adjustment) < EVENT_1_TIME
                           ? std::vector<std::string>{"Finished"}
                           : std::vector<std::string>{"Event 1", "Finished"};
            break;
        default: throw std::runtime_error("Unknown action");
    }

    INFO(test_util::diff_string(expected, reactor.events));
    CHECK(reactor.events == expected);
}
