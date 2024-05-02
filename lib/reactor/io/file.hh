#include "io.hh"

namespace corey {

class File {
public:

    static Future<File> open(const char* path, int flags);
    static Future<File> open(const char* path, int flags, mode_t mode);

    File() noexcept;
    File(const File& other) = delete;
    File& operator=(const File& other) = delete;
    File(File&& other) noexcept;
    File& operator=(File&& other) noexcept;
    ~File();

    Future<> fsync() const;
    Future<> fdatasync() const;
    Future<uint64_t> read(uint64_t offset, std::span<char>) const;
    Future<uint64_t> write(uint64_t offset, std::span<const char>) const;
    Future<> close();

private:
    File(int fd);

    int _fd;
};

} // namespace corey