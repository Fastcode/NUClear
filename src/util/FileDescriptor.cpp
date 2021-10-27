#include "FileDescriptor.hpp"

namespace NUClear {
namespace util {

    FileDescriptor::FileDescriptor() = default;

    FileDescriptor::FileDescriptor(const fd_t& fd) : fd(fd) {}

    FileDescriptor::~FileDescriptor() {
        close_fd();
    }

    FileDescriptor::FileDescriptor(FileDescriptor&& rhs) noexcept : fd{rhs.fd} {
        if (this != &rhs) { rhs.fd = INVALID_SOCKET; }
    }

    FileDescriptor& FileDescriptor::operator=(FileDescriptor&& rhs) noexcept {
        if (this != &rhs) {
            close_fd();
            fd     = rhs.get();
            rhs.fd = INVALID_SOCKET;
        }
        return *this;
    }

    // No Lint: As we are giving access to a variable which can change state.
    // NOLINTNEXTLINE(readability-make-member-function-const)
    fd_t FileDescriptor::get() {
        return fd;
    }

    bool FileDescriptor::valid() const {
        return fd >= 0;
    }

    fd_t FileDescriptor::release() {
        fd_t temp = fd;
        fd        = INVALID_SOCKET;
        return temp;
    }

    FileDescriptor::operator fd_t() {
        return fd;
    }

    void FileDescriptor::close_fd() {
        if (fd != INVALID_SOCKET) { close(fd); }
        fd = INVALID_SOCKET;
    }

}  // namespace util
}  // namespace NUClear
