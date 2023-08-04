#include "FileDescriptor.hpp"

#include <utility>

#include "platform.hpp"

namespace NUClear {
namespace util {


    FileDescriptor::FileDescriptor() = default;
    FileDescriptor::FileDescriptor(const int& fd_, std::function<void(fd_t)> cleanup_)
        : fd(fd_), cleanup(std::move(cleanup_)) {}

    FileDescriptor::~FileDescriptor() {
        close();
    }

    void FileDescriptor::close() {
        if (valid()) {
            if (cleanup) {
                cleanup(fd);
            }
            ::close(fd);
            fd = INVALID_SOCKET;
        }
    }

    FileDescriptor::FileDescriptor(FileDescriptor&& rhs) noexcept : fd{rhs.fd} {
        if (this != &rhs) {
            rhs.fd = INVALID_SOCKET;
        }
    }

    FileDescriptor& FileDescriptor::operator=(FileDescriptor&& rhs) noexcept {
        if (this != &rhs) {
            close();
            fd = std::exchange(rhs.fd, INVALID_SOCKET);
        }
        return *this;
    }

    // No Lint: As we are giving access to a variable which can change state.
    // NOLINTNEXTLINE(readability-make-member-function-const)
    int FileDescriptor::get() {
        return fd;
    }

    bool FileDescriptor::valid() const {
#ifdef _WIN32
        return fd != INVALID_SOCKET;
#else
        return ::fcntl(fd, F_GETFL) != -1 || errno != EBADF;
#endif
    }

    fd_t FileDescriptor::release() {
        return std::exchange(fd, INVALID_SOCKET);
    }

    // Should not be const as editing the file descriptor would change the state
    // NOLINTNEXTLINE(readability-make-member-function-const)
    FileDescriptor::operator fd_t() {
        return fd;
    }

}  // namespace util
}  // namespace NUClear
