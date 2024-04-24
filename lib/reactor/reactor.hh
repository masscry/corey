#pragma once

#include "task.hh"
#include "future.hh"

#include <list>

namespace corey {

class Reactor {
    using TaskList = std::list<Task>;
public:

    Reactor();
    Reactor(const Reactor&) = delete;
    Reactor& operator=(const Reactor&) = delete;
    Reactor(Reactor&&);
    Reactor& operator=(Reactor&&);
    ~Reactor();

    void run();
    void add(Task&&);

private:
    TaskList _tasks;
};

} // namespace corey
