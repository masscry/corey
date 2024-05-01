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
    for (auto& routine: _routines) {
        routine.second.execute();
    }
    while (!_tasks.empty()) {
        _tasks.front().execute_and_dispose();
    }
}

void Reactor::add(Task&& task) {
    _tasks.push_back(*new Task(std::move(task)));
}

Defer<> Reactor::add(Routine&& routine) {
    uint16_t id = _routines.size();
    auto start = id;
    while(_routines.count(id++)) { COREY_ASSERT(id != start); }
    _routines[id] = std::move(routine);
    return defer([this, id]() noexcept { remove_routine(id);});
}

void Reactor::remove_routine(int id) {
    _routines.erase(id);
}

} // namespace corey
