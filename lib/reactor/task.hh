#pragma once

#include <utility>
#include <memory>
#include <span>

#include <boost/intrusive/link_mode.hpp>
#include <boost/intrusive/list.hpp>

namespace corey {

struct Executable {
    virtual void execute() = 0;
    virtual ~Executable() = 0;
};
using UPExecutable = std::unique_ptr<Executable>;

using AutoLinkBase = boost::intrusive::list_base_hook<boost::intrusive::link_mode<boost::intrusive::auto_unlink>>;

class Task : public AutoLinkBase {
public:

    Task() = default;
    Task(const Task& other) = delete;
    Task& operator=(const Task& other) = delete;
    
    Task(Task&& other) noexcept = default;
    Task& operator=(Task&& other) noexcept = default;
    ~Task() = default;

    void execute_and_dispose() noexcept {
        _model->execute();
        delete this;
    }

    void execute_once() {
        _model->execute();
        _model.reset();
    }

    bool is_good() const noexcept {
        return static_cast<bool>(_model);
    }

    Executable& get_impl() noexcept { return *_model; }

    template<typename Impl, typename... Args>
    static Task make(Args&&... args) {
        return Task(std::make_unique<Impl>(std::forward<Args>(args)...));
    }

private:
    explicit Task(UPExecutable&& model) noexcept : _model(std::move(model)) {}
    UPExecutable _model;
};

class Routine {
public:
    Routine() = default;
    Routine(const Routine& other) = delete;
    Routine& operator=(const Routine& other) = delete;
    
    Routine(Routine&& other) noexcept = default;
    Routine& operator=(Routine&& other) noexcept = default;
    ~Routine() = default;

    void execute() { _model->execute(); }

    Executable& get_impl() noexcept { return *_model; }

    template<typename Impl, typename... Args>
    static Routine make(Args&&... args) {
        return Routine(std::make_unique<Impl>(std::forward<Args>(args)...));
    }

private:
    explicit Routine(UPExecutable&& model) noexcept : _model(std::move(model)) {}
    UPExecutable _model;
};

template<typename Func>
auto make_task(Func&& func) {
    struct Lambda : public Executable{
        Func _func;
        Lambda(Func&& func) : _func(std::forward<Func>(func)) {}
        void execute() override { _func(); }
    };
    return Task::make<Lambda>(std::forward<Func>(func));
}

template<typename Func>
auto make_routine(Func&& func) {
    struct Lambda : public Executable {
        Func _func;
        Lambda(Func&& func) : _func(std::forward<Func>(func)) {}
        void execute() override { _func(); }
    };
    return Routine::make<Lambda>(std::forward<Func>(func));
}

} // namespace corey
