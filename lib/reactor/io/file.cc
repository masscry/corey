#include "file.hh"

#include "reactor/coroutine.hh"

namespace corey {

namespace {

Log logger("io/file");

} // namespace

Future<File> File::open(const char* path, int flags) {
    return open(path, flags, 0);
}

Future<File> File::open(const char* path, int flags, mode_t mode) {
    auto fd = co_await IoEngine::instance().open(path, flags, mode);
    if (fd < 0) {
        co_await std::make_exception_ptr(std::system_error(-fd, std::system_category(), "open failed"));
    }
    co_return File(fd);
}

constexpr int invalid_fd = -1;

File::File() noexcept : File(invalid_fd) { logger.debug("File()"); }
File::File(int fd) : _fd(fd) { logger.debug("File({})", fd); }

File::File(File&& other) noexcept: _fd(other._fd) {
    other._fd = invalid_fd;
}

File& File::operator=(File&& other) noexcept {
    if (this != &other) {
        this->~File();
        new (this) File(std::move(other));
    }
    return *this;
}

File::~File() {
    if (_fd != invalid_fd) {
        panic("File not closed");
    }
}

Future<> File::fsync() {
    auto ret = co_await IoEngine::instance().fsync(_fd);
    if (ret < 0) {
        co_await std::make_exception_ptr(std::system_error(-ret, std::system_category(), "fsync failed"));
    }
}

Future<> File::fdatasync() {
    auto ret = co_await IoEngine::instance().fdatasync(_fd);
    if (ret < 0) {
        co_await std::make_exception_ptr(std::system_error(-ret, std::system_category(), "fdatasync failed"));
    }
}

Future<> File::read(uint64_t offset, std::span<char> data) {
    auto result = co_await IoEngine::instance().read(_fd, offset, data);
    if (result < 0) {
        co_await std::make_exception_ptr(std::system_error(-result, std::system_category(), "read failed"));
    }
    if (static_cast<uint64_t>(result) != data.size()) {
        co_await std::make_exception_ptr(std::runtime_error(fmt::format("read failed: expected {} bytes, got {}", data.size(), result)));
    }
}

Future<> File::write(uint64_t offset, std::span<const char> data) {
    auto result = co_await IoEngine::instance().write(_fd, offset, data);
    if (result < 0) {
        co_await std::make_exception_ptr(std::system_error(-result, std::system_category(), "write failed"));
    }
    if (static_cast<uint64_t>(result) != data.size()) {
        co_await std::make_exception_ptr(std::runtime_error(fmt::format("write failed: expected {} bytes, got {}", data.size(), result)));
    }
}

Future<> File::close() && {
    if (_fd == -1) {
        co_await std::make_exception_ptr(std::runtime_error("File already closed"));
    }
    auto result = co_await IoEngine::instance().close(_fd);
    _fd = -1;
    if (result < 0) {
        co_await std::make_exception_ptr(std::system_error(-result, std::system_category(), "close failed"));
    }
}

} // namespace corey
