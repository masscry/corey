#pragma once

#include <concepts>
#include <utility>
#include <type_traits>
#include <memory>

namespace corey {

template<typename Func = void>
class [[nodiscard]] Defer {
public:
    Defer(Func&& func) noexcept : _func(std::forward<Func>(func)) {
        static_assert(std::is_nothrow_invocable_v<Func>, "Func must invocable without throwing exceptions.");
    }
    Defer(const Defer&) = delete;
    Defer& operator=(const Defer&) = delete;
    Defer(Defer&& other) noexcept
        : _func(std::move(other._func))
        , _cancelled(other._cancelled) {
        other._cancelled = true;
    }
    Defer& operator=(Defer&& other) noexcept {
        if (this != &other) {
            this->~Defer();
            new (this) Defer(std::move(other));
        }
        return *this;
    }
    ~Defer() {
        if (!_cancelled) {
            _func();
            _cancelled = true;
        } 
    }

    void cancel() noexcept { _cancelled = true; }

private:
    Func _func;
    bool _cancelled = false;
};

template<>
class [[nodiscard]] Defer<void> {
    class Impl {
    public:
        virtual ~Impl() = default;
        virtual void cancel() noexcept = 0;
    };

    template<typename Func>
    struct DeferImpl : public Impl {
        DeferImpl(Defer<Func>&& other) noexcept : _func(std::move(other)) { ; }
        void cancel() noexcept override { _func.cancel(); }
        Defer<Func> release() && { return std::move(_func); }
        Defer<Func> _func;
    };

public:
    Defer() noexcept = default;
    Defer(const Defer<void>&) = delete;
    Defer& operator=(const Defer<void>&) = delete;
    Defer(Defer<void>&&) noexcept = default;
    Defer& operator=(Defer<void>&&) noexcept = default;
    ~Defer() = default;

    template<typename Func>
    Defer(Defer<Func>&& other) noexcept
        : _impl(std::make_unique<DeferImpl<Func>>(std::move(other)))  {}

    template<typename Func>
    Defer<>& operator=(Defer<Func>&& other) noexcept {
        this->~Defer();
        new (this) Defer(std::move(other));
        return *this;
    }

    template<typename Func>
    operator Defer<Func>&() const noexcept {
        return static_cast<DeferImpl<Func>*>(_impl.get())->_func;
    }

    void cancel() noexcept { _impl->cancel(); }
private:
    std::unique_ptr<Impl> _impl;
};

template<typename Func>
requires std::is_nothrow_invocable_v<Func>
[[nodiscard]]
auto defer(Func&& func) {
    return Defer<Func>(std::forward<Func>(func));
}

} // namespace corey
