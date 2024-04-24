#pragma once

#include <cstddef>

#include <span>

namespace corey {

class Console {
public:
    std::size_t write(std::span<char>) const;
};

} // namespace corey
