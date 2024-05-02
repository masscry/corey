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

    Future<> fsync();
    Future<> fdatasync();
    Future<> read(uint64_t offset, std::span<char>);
    Future<> write(uint64_t offset, std::span<const char>);
    Future<> close() &&;

    void seek(std::size_t pos);
    std::size_t tell() const;

private:
    File(int fd);

    int _fd;
};

} // namespace corey