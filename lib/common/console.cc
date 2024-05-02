#include "console.hh"

#include <cstdio>
#include <system_error>

namespace corey {

std::size_t Console::write(std::span<const char> buf) const {
    auto result = fwrite(buf.data(), sizeof(char), buf.size(), stdout);
    if (result != buf.size()) {
        throw std::system_error(errno, std::generic_category(), "console");
    }
    return result;
}

} // namespace corey
