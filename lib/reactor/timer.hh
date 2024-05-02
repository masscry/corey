#pragma once

#include "io.hh"
#include "reactor.hh"

#include <chrono>

namespace corey {

using namespace std::chrono_literals;

Future<> sleep(std::chrono::nanoseconds);

} // namespace corey
