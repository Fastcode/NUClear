#ifndef NUCLEAR_INTERNAL_EVERY_H
#define NUCLEAR_INTERNAL_EVERY_H
#include <chrono>
namespace NUClear {
    namespace Internal {
        template <int ticks, class period = std::chrono::milliseconds>
        class Every {};
    }
}
#endif
