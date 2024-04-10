#define CATCH_CONFIG_MAIN
#include <catch.hpp>
#include <future>
#include <nuclear>

#include "test_util/TestBase.hpp"

namespace {

static constexpr std::chrono::milliseconds EVENT_1_TIME = std::chrono::milliseconds(4);
static constexpr std::chrono::milliseconds EVENT_2_TIME = std::chrono::milliseconds(8);

struct WaitForShutdown {};

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

    // Events
    std::vector<std::string> events = {};

    TestReactor(std::unique_ptr<NUClear::Environment> environment) : TestBase(std::move(environment), false) {

        on<Startup>().then([this] {
            // Emit a chrono task to run at time EVENT_1_TIME
            emit<Scope::DIRECT>(std::make_unique<NUClear::dsl::operation::ChronoTask>(
                [this](NUClear::clock::time_point&) {
                    events.push_back("Event 1");
                    return false;
                },
                NUClear::clock::now() + EVENT_1_TIME,
                1));

            // Emit a chrono task to run at time EVENT_2_TIME
            emit<Scope::DIRECT>(std::make_unique<NUClear::dsl::operation::ChronoTask>(
                [this](NUClear::clock::time_point&) {
                    events.push_back("Event 2");
                    return false;
                },
                NUClear::clock::now() + EVENT_2_TIME,
                2));

            // Time travel
            emit<Scope::DIRECT>(std::make_unique<NUClear::message::TimeTravel>(adjustment, rtf, action));

            // Shutdown after steady clock amount of time
            emit(std::make_unique<WaitForShutdown>());
        });

        on<Trigger<WaitForShutdown>>().then([this] {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            events.push_back("Finished");
            powerplant.shutdown();
        });
    }
};

}  // anonymous namespace

TEST_CASE("Test time travel correctly changes the time for non zero rtf", "[time_travel][chrono_controller]") {

    using Action = NUClear::message::TimeTravel::Action;

    const NUClear::Configuration config;
    auto plant    = std::make_shared<NUClear::PowerPlant>(config);
    auto& reactor = plant->install<TestReactor>();

    // Set the reactor fields to the values we want to test
    Action action      = GENERATE(Action::RELATIVE, Action::ABSOLUTE, Action::NEAREST);
    int64_t adjustment = GENERATE(-4, -2, 0, 2, 4, 6, 8, 10);
    CAPTURE(action, adjustment);
    reactor.action     = action;
    reactor.adjustment = std::chrono::milliseconds(adjustment);
    reactor.rtf        = 0.0;

    // Reset clock to zero
    NUClear::clock::set_clock(NUClear::clock::time_point());

    // Start the powerplant
    plant->start();

    // Expected results
    std::vector<std::string> expected_events;
    switch (action) {
        case Action::RELATIVE: expected_events = {"Finished"}; break;
        case Action::ABSOLUTE:
            if (std::chrono::milliseconds(adjustment) < EVENT_1_TIME) {
                expected_events = {"Finished"};
            }
            else if (std::chrono::milliseconds(adjustment) < EVENT_2_TIME) {
                expected_events = {"Event 1", "Finished"};
            }
            else {
                expected_events = {"Event 1", "Event 2", "Finished"};
            }
            break;
        case Action::NEAREST:
            expected_events = std::chrono::milliseconds(adjustment) < EVENT_1_TIME
                                  ? std::vector<std::string>{"Finished"}
                                  : std::vector<std::string>{"Event 1", "Finished"};
            break;
        default: throw std::runtime_error("Unknown action");
    }

    const auto& actual_events = reactor.events;
    CHECK(expected_events == actual_events);
}
