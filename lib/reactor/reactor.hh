#pragma once

#include "reactor/task.hh"
#include "reactor/future.hh"
#include "common/defer.hh"

#include <boost/intrusive/list.hpp>
#include <boost/container/flat_map.hpp>

namespace corey {

class Reactor {
    using TaskList = boost::intrusive::list<Task, boost::intrusive::constant_time_size<false>>;
    using RoutineList = boost::container::flat_map<int, Routine>;
public:

    static
    Reactor& instance();

    Reactor();
    Reactor(const Reactor&) = delete;
    Reactor& operator=(const Reactor&) = delete;
    Reactor(Reactor&&) = delete;
    Reactor& operator=(Reactor&&) = delete;
    ~Reactor();

    void run();
    void add(Task&&);
    Defer<> add(Routine&&);

private:

    void remove_routine(int id);

    TaskList _tasks;
    RoutineList _routines;
};

} // namespace corey
