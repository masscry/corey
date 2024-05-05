#pragma once

#include <utility>
#include <memory>
#include <span>

#include <boost/intrusive/link_mode.hpp>
#include <boost/intrusive/list.hpp>

namespace corey {

struct AbstractExecutable {
    virtual bool execute() = 0;
    virtual ~AbstractExecutable() = 0;
};
using UPAbstractExecutable = std::unique_ptr<AbstractExecutable>;

using AutoLinkBase = boost::intrusive::list_base_hook<boost::intrusive::link_mode<boost::intrusive::auto_unlink>>;

class Executable : public AutoLinkBase {
public:

    Executable() = default;
    Executable(const Executable& other) = delete;
    Executable& operator=(const Executable& other) = delete;
    
    Executable(Executable&& other) noexcept = default;
    Executable& operator=(Executable&& other) noexcept = default;
    ~Executable() = default;

    bool try_execute() {
        return _model->execute();
    }

    AbstractExecutable& get_impl() noexcept { return *_model; }

    template<typename Impl, typename... Args>
    static Executable make(Args&&... args) {
        return Executable(std::make_unique<Impl>(std::forward<Args>(args)...));
    }

private:
    explicit Executable(UPAbstractExecutable&& model) noexcept : _model(std::move(model)) {}
    UPAbstractExecutable _model;
};

template<typename Func>
auto make_task(Func&& func) {
    struct SimpleTask final : public AbstractExecutable{
        Func _func;
        SimpleTask(Func&& func) : _func(std::forward<Func>(func)) {}
        bool execute() override {
            _func();
            return true;
        }
    };
    return Executable::make<SimpleTask>(std::forward<Func>(func));
}

template<typename ExeFunc, typename IsReadyFunc>
auto make_task(ExeFunc&& exeFunc, IsReadyFunc&& isReadyFunc) {
    struct MaybeReadyTask final : public AbstractExecutable {
        ExeFunc _exeFunc;
        IsReadyFunc _isReadyFunc;
        MaybeReadyTask(ExeFunc&& exeFunc, IsReadyFunc&& isReadyFunc)
            : _exeFunc(std::forward<ExeFunc>(exeFunc))
            , _isReadyFunc(std::forward<IsReadyFunc>(isReadyFunc)) {}
        bool execute() override {
            if (!_isReadyFunc()) {
                return false;
            }
            _exeFunc();
            return true;
        }
    };
    return Executable::make<MaybeReadyTask>(
        std::forward<ExeFunc>(exeFunc),
        std::forward<IsReadyFunc>(isReadyFunc)
    );
}

template<typename Func>
auto make_routine(Func&& func) {
    struct SimpleRoutine final : public AbstractExecutable {
        Func _func;
        SimpleRoutine(Func&& func) : _func(std::forward<Func>(func)) {}
        bool execute() override {
            _func();
            return false;
        }
    };
    return Executable::make<SimpleRoutine>(std::forward<Func>(func));
}

} // namespace corey
