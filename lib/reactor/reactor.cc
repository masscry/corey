#include "reactor.hh"
#include "utils/common.hh"

namespace corey {

Reactor::Reactor() = default;
Reactor::Reactor(Reactor&&) = default;

Reactor::~Reactor() {
    COREY_ASSERT(_tasks.empty());
}

Reactor& Reactor::operator=(Reactor&&) = default;

void Reactor::run() {
    for (const auto& task: _tasks) {
        task.execute();
    }
    _tasks.clear();
}

void Reactor::add(Task&& task) {
    _tasks.emplace_back(std::move(task));
}

} // namespace corey
