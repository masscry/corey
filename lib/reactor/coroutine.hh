#pragma once

#include "reactor/future.hh"
#include "reactor/reactor.hh"
#include "reactor/task.hh"
#include "utils/log.hh"

#include <coroutine>
#include <exception>
#include <type_traits>

namespace corey {

struct Yield {};

inline constexpr Yield yield() { return {}; }

template<typename Self>
struct BaseCoroPromise {
    auto get_return_object() {
        return static_cast<Self*>(this)->_promise.get_future();
    }

    [[nodiscard]] constexpr std::suspend_never initial_suspend() noexcept { return {}; }
    [[nodiscard]] constexpr std::suspend_never final_suspend() noexcept { return {}; }

    void unhandled_exception() {
        if (!static_cast<Self*>(this)->_promise.has_future()) {
            log_orphaned_exception(std::current_exception());
        }
        static_cast<Self*>(this)->_promise.set_exception(std::current_exception());
    }

    template<typename FutData>
    auto await_transform(corey::Future<FutData>&& future) {
        struct Awaiter {
            corey::Future<FutData> future;
            bool await_ready() const noexcept { return future.is_ready(); }
            void await_suspend(std::coroutine_handle<> handle) noexcept {
                corey::Reactor::instance().add(make_task(
                    [handle]() { handle.resume(); },
                    [this]() { return future.is_ready(); }
                ));
            }
            FutData await_resume() {
                return future.get();
            }
        };
        return Awaiter{ std::move(future) };
    }

    auto await_transform(std::exception_ptr exp) {
        struct Awaiter {
            constexpr bool await_ready() const noexcept { return false; }
            void await_suspend(std::coroutine_handle<> handle) noexcept {
                handle.destroy();
            }
            constexpr void await_resume() noexcept {}
        };
        if (!static_cast<Self*>(this)->_promise.has_future()) {
            log_orphaned_exception(std::current_exception());
        }
        static_cast<Self*>(this)->_promise.set_exception(exp);
        return Awaiter{};
    }

    auto await_transform(Yield) {
        struct Awaiter {
            constexpr bool await_ready() const noexcept { return false; }
            void await_suspend(std::coroutine_handle<> handle) noexcept {
                corey::Reactor::instance().add(make_task([handle]() { handle.resume(); }));
            }
            constexpr void await_resume() noexcept {}
        };
        return Awaiter{};
    }

};

template<typename Data>
struct CoroPromise : public BaseCoroPromise<CoroPromise<Data>> {
    Promise<Data> _promise;
    void return_value(Data&& data) noexcept(std::is_nothrow_move_constructible_v<Data>) {
        _promise.set(std::move(data));
    }
    void return_value(const Data& data) noexcept(std::is_nothrow_copy_constructible_v<Data>) {
        _promise.set(data);
    }
    void return_value(std::exception_ptr exp) noexcept {
        if (!_promise.has_future()) {
            log_orphaned_exception(std::current_exception());
        }
        _promise.set_exception(exp);
    }
};

template<>
struct CoroPromise<void> : public BaseCoroPromise<CoroPromise<void>> {
    Promise<> _promise;
    void return_void() noexcept {
        _promise.set();
    }
};

} // namespace corey

template<typename Data, typename... Args>
struct std::coroutine_traits<corey::Future<Data>, Args...> {
    using promise_type = corey::CoroPromise<Data>;
};
