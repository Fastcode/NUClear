#include "thread_pool.hpp"

namespace NUClear {
namespace util {

    std::atomic<uint64_t> ThreadPoolIDSource::source = {2};

}  // namespace util
}  // namespace NUClear
