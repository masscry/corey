#pragma once

#include <cstddef>

#include <span>

namespace corey {

class Console {
public:
    std::size_t write(std::span<const char>) const;
};

} // namespace corey
