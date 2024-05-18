#pragma once

#include <utility>
#include <memory>
#include <span>

#include <boost/intrusive/link_mode.hpp>
#include <boost/intrusive/list.hpp>

namespace corey {

template<typename Ret, typename... Args>
struct AbstractExecutable {
    virtual Ret execute(Args&&...) = 0;
    virtual ~AbstractExecutable() = 0;
};

template <typename Ret, typename... Args>
AbstractExecutable<Ret, Args...>::~AbstractExecutable() = default;

template<typename Ret, typename... Args>
using UPAbstractExecutable = std::unique_ptr<AbstractExecutable<Ret, Args...>>;

using AutoLinkBase = boost::intrusive::list_base_hook<boost::intrusive::link_mode<boost::intrusive::auto_unlink>>;

template<typename Ret, typename... Args>
class Executable : public AutoLinkBase {
public:

    Executable() = default;
    Executable(const Executable& other) = delete;
    Executable& operator=(const Executable& other) = delete;
    
    Executable(Executable&& other) noexcept = default;
    Executable& operator=(Executable&& other) noexcept = default;
    ~Executable() = default;

    Ret operator()(Args&&... args) {
        return _model->execute(std::forward<Args>(args)...);
    }

    AbstractExecutable<Ret, Args...>& get_impl() noexcept { return *_model; }

    template<typename Impl, typename... MakeArgs>
    static Executable make(MakeArgs&&... args) {
        return Executable(std::make_unique<Impl>(std::forward<MakeArgs>(args)...));
    }

private:
    explicit Executable(UPAbstractExecutable<Ret, Args...>&& model) noexcept : _model(std::move(model)) {}
    UPAbstractExecutable<Ret, Args...> _model;
};

using Task = Executable<bool>;
using Routine = Executable<void>;

template<typename Func>
auto make_task(Func&& func) {
    struct SimpleTask final : public AbstractExecutable<bool> {
        Func _func;
        SimpleTask(Func&& func) : _func(std::forward<Func>(func)) {}
        bool execute() override {
            _func();
            return true;
        }
    };
    return Task::make<SimpleTask>(std::forward<Func>(func));
}

template<typename ExeFunc, typename IsReadyFunc>
auto make_task(ExeFunc&& exeFunc, IsReadyFunc&& isReadyFunc) {
    struct MaybeReadyTask final : public AbstractExecutable<bool> {
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
    return Task::make<MaybeReadyTask>(
        std::forward<ExeFunc>(exeFunc),
        std::forward<IsReadyFunc>(isReadyFunc)
    );
}

template<typename Func>
auto make_routine(Func&& func) {
    struct SimpleRoutine final : public AbstractExecutable<void> {
        Func _func;
        SimpleRoutine(Func&& func) : _func(std::forward<Func>(func)) {}
        void execute() override {
            _func();
        }
    };
    return Routine::make<SimpleRoutine>(std::forward<Func>(func));
}

} // namespace corey
