#include "thread_pool.hpp"

namespace NUClear {
namespace util {

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
    std::atomic<uint64_t> ThreadPoolIDSource::source = {2};

}  // namespace util
}  // namespace NUClear
