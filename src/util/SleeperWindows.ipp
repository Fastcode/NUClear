
namespace NUClear {
namespace util {

    struct SleeperState {
        SleeperState() {
            // Create a waitable timer and an event to wake up the sleeper prematurely
            timer = ::CreateWaitableTimer(nullptr, TRUE, nullptr);
            waker = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
        }
        ~SleeperState() {
            ::CloseHandle(timer);
            ::CloseHandle(waker);
        }
        ::HANDLE timer;
        ::HANDLE waker;
    };

    void Sleeper::sleep_until(const std::chrono::steady_clock& target) {
        auto now = std::chrono::steady_clock::now();

        if (now - target > std::chrono::nanoseconds(0)) {
            return;
        }

        auto ns = target - now;
        ::LARGE_INTEGER ft;
        // TODO if ns is negative make it 0 as otherwise it'll become absolute time
        // Negative for relative time, positive for absolute time
        // Measures in 100ns increments so divide by 100
        ft.QuadPart = -static_cast<int64_t>(ns.count() / 100);

        ::SetWaitableTimer(state->timer, &ft, 0, nullptr, nullptr, 0);
        std::array<const ::HANDLE, 2> items = {state->timer, state->waker};
        ::WaitForMultipleObjects(2, items, FALSE, INFINITE);
    }

}  // namespace util
}  // namespace NUClear
