#include "FileDescriptor.hpp"

namespace NUClear {
namespace util {

    // Constructor: Default
    FileDescriptor::FileDescriptor() = default;

    // Constructor: fd_t
    FileDescriptor::FileDescriptor(const fd_t& fd) : fd(fd) {}

    // Destructor:
    FileDescriptor::~FileDescriptor() {
        close_fd();
    }

    // Moving:
    FileDescriptor::FileDescriptor(FileDescriptor&& rhs) noexcept : fd{rhs.fd} {
        if (this != &rhs) { rhs.fd = INVALID_SOCKET; }
    }

    // Moving: Operator
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

    /**
     * @brief Releases the file descriptor from RAII.
     * @details This releases and returns the file descriptor
     *          it holds. This allows it to no longer be
     *          subject to closure on deletion.
     *          After this this FileDescriptor object will
     *          be invalidated.
     * @return the held file descriptor
     */
    fd_t FileDescriptor::release() {
        fd_t temp = fd;
        fd        = INVALID_SOCKET;
        return temp;
    }

    /**
     * @brief Casts this to the held file descriptor
     *
     * @return the file descriptor
     */
    FileDescriptor::operator fd_t() {
        return fd;
    }

    /**
     * @brief Close the current file descriptor, if it's valid
     */
    void FileDescriptor::close_fd() {
// On windows it's CLOSE_SOCKET
#if defined(_WIN32)
        if (fd != INVALID_SOCKET) { close(fd); }
// On others we just close
#else
        if (fd > 0) { close(fd); }
#endif
        fd = INVALID_SOCKET;
    }

}  // namespace util
}  // namespace NUClear
