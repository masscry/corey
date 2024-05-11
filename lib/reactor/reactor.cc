#include "reactor/reactor.hh"
#include "reactor/coroutine.hh"
#include "utils/common.hh"

namespace corey {

namespace {

Reactor* g_instance = nullptr;

}

Reactor& Reactor::instance() {
    if (!g_instance) {
        panic("No active reactor");
    }
    return *g_instance;
}

Reactor::Reactor() : _has_progress(false) {
    COREY_ASSERT(!g_instance);
    g_instance = this;
}

Reactor::~Reactor() {
    COREY_ASSERT(_tasks.empty());
    COREY_ASSERT(_routines.empty());
    g_instance = nullptr;
}

void Reactor::run() {
    for (auto& routine: _routines) {
        routine.second.try_execute();
    }

    bool new_has_progress = false;
    for (auto task = _tasks.begin(); task != _tasks.end();) {
        auto next = std::next(task);
        if (task->try_execute()) {
            _tasks.erase_and_dispose(task, [](Executable* task) { delete task; });
            new_has_progress = true;
        }
        task = next;
    }
    _has_progress = new_has_progress;
}

void Reactor::add_task(Executable&& task) {
    _tasks.push_back(*new Executable(std::move(task)));
}

Defer<> Reactor::add_routine(Executable&& routine) {
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
