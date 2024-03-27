#define CATCH_CONFIG_MAIN
#include <catch.hpp>
#include <nuclear>

#include "test_util/TestBase.hpp"

namespace {

// Helper struct for testing chrono tasks
struct ChronoTaskTest {
    NUClear::clock::time_point time   = NUClear::clock::time_point();  // Time to execute the task
    bool ran                          = false;                         // Whether the task has been executed
    std::chrono::seconds steady_delta = std::chrono::seconds(0);       // Real time delta
    std::chrono::seconds system_delta = std::chrono::seconds(0);       // System time delta
    NUClear::id_t id                  = 0;                             // Unique identifier for the task
    ChronoTaskTest() {}
    ChronoTaskTest(NUClear::clock::time_point time_, bool ran_, NUClear::id_t id_) : time(time_), ran(ran_), id(id_) {}
};

std::unique_ptr<ChronoTaskTest> test_task;

NUClear::clock::duration time_travel_adjustment;
NUClear::message::TimeTravel::Action time_travel_action;
std::chrono::seconds chrono_task_delay;

std::chrono::steady_clock::time_point steady_start_time;
NUClear::clock::time_point system_start_time;

class TestReactor : public test_util::TestBase<TestReactor, 5000> {
public:
    TestReactor(std::unique_ptr<NUClear::Environment> environment) : TestBase(std::move(environment), false) {

        on<Startup>().then([this] {
            // Emit a chrono task to run after a delay
            test_task        = std::make_unique<ChronoTaskTest>(NUClear::clock::now() + chrono_task_delay, false, 1);
            auto chrono_task = std::make_unique<NUClear::dsl::operation::ChronoTask>(
                [this](NUClear::clock::time_point&) {
                    test_task->ran       = true;
                    auto steady_end_time = std::chrono::steady_clock::now();
                    test_task->steady_delta =
                        std::chrono::duration_cast<std::chrono::seconds>(steady_end_time - steady_start_time);
                    auto system_end_time = NUClear::clock::now();
                    test_task->system_delta =
                        std::chrono::duration_cast<std::chrono::seconds>(system_end_time - system_start_time);
                    powerplant.shutdown();
                    return false;  // Do not repeat the test_task
                },
                test_task->time,
                test_task->id);
            emit<Scope::DIRECT>(std::move(chrono_task));

            // Time travel!
            emit<Scope::DIRECT>(
                std::make_unique<NUClear::message::TimeTravel>(time_travel_adjustment, 1.0, time_travel_action));
        });
    }
};

void perform_time_travel(const NUClear::message::TimeTravel::Action action,
                         const NUClear::clock::duration adjustment,
                         const std::chrono::seconds& shutdown_delay,
                         const std::chrono::seconds& task_delay) {
    NUClear::Configuration config;
    NUClear::PowerPlant plant(config);
    time_travel_action     = action;
    time_travel_adjustment = adjustment;
    chrono_task_delay      = task_delay;

    // Reset the clock
    NUClear::clock::set_clock(NUClear::clock::time_point());
    plant.install<TestReactor>();

    // Record the start time for NUClear::clock and std::chrono::steady_clock
    steady_start_time = std::chrono::steady_clock::now();
    system_start_time = NUClear::clock::now();

    // Spin off a separate thread to shutdown the powerplant after a steady clock delay
    std::thread([shutdown_delay, &plant] {
        std::this_thread::sleep_for(shutdown_delay);
        plant.shutdown();
    }).detach();
    plant.start();
}

}  // anonymous namespace

TEST_CASE("TimeTravel Actions Test", "[time_travel][chrono_controller]") {
    SECTION("Action::RELATIVE with shutdown before task time") {
        perform_time_travel(NUClear::message::TimeTravel::Action::RELATIVE,
                            std::chrono::seconds(3),   // Time travel adjustment
                            std::chrono::seconds(2),   // Shutdown delay
                            std::chrono::seconds(3));  // Chrono task delay
        INFO("Task expected ran: " << 0 << ", actual ran: " << test_task->ran);
        REQUIRE_FALSE(test_task->ran);
    }

    SECTION("Action::RELATIVE with shutdown after task time") {
        perform_time_travel(NUClear::message::TimeTravel::Action::RELATIVE,
                            std::chrono::seconds(3),   // Time travel adjustment
                            std::chrono::seconds(3),   // Shutdown delay
                            std::chrono::seconds(2));  // Chrono task delay
        INFO("Task expected ran: " << 1 << ", actual ran: " << test_task->ran);
        REQUIRE(test_task->ran);
    }

    SECTION("Action::JUMP with task time before jump time") {
        perform_time_travel(NUClear::message::TimeTravel::Action::JUMP,
                            std::chrono::seconds(3),   // Time travel adjustment
                            std::chrono::seconds(4),   // Shutdown delay
                            std::chrono::seconds(3));  // Chrono task delay
        INFO("Task expected ran: " << 1 << ", actual ran: " << test_task->ran);
        REQUIRE(test_task->ran);
        REQUIRE(test_task->steady_delta.count() == 0);
        REQUIRE(test_task->system_delta.count() == 3);
    }

    SECTION("Action::JUMP with task time after jump time") {
        perform_time_travel(NUClear::message::TimeTravel::Action::JUMP,
                            std::chrono::seconds(3),   // Time travel adjustment
                            std::chrono::seconds(2),   // Shutdown delay
                            std::chrono::seconds(5));  // Chrono task delay
        INFO("Task expected ran: " << 0 << ", actual ran: " << test_task->ran);
        REQUIRE_FALSE(test_task->ran);
    }

    SECTION("Action::NEAREST") {
        perform_time_travel(NUClear::message::TimeTravel::Action::NEAREST,
                            std::chrono::seconds(5),   // Time travel adjustment
                            std::chrono::seconds(10),  // Shutdown delay
                            std::chrono::seconds(1));  // Chrono task delay
        INFO("Task expected ran: " << 1 << ", actual ran: " << test_task->ran);
        REQUIRE(test_task->ran);
        REQUIRE(test_task->steady_delta.count() == 0);
        REQUIRE(test_task->system_delta.count() == 1);
    }
}
