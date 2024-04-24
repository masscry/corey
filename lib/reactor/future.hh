#pragma once

#include "utils/common.hh"

#include <algorithm>
#include <array>
#include <coroutine>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <stdexcept>
#include <type_traits>

#include <boost/smart_ptr/intrusive_ptr.hpp>

namespace corey {

template<typename Ptr>
using IntrusivePtr = boost::intrusive_ptr<Ptr>;

class FutureException: public std::exception {
public:
    enum class Type {
        broken_promise = 0,
        already_retrived,
        already_satisfied,
        not_ready
    };
    FutureException(Type type) : _type(type) {;}
    FutureException(const FutureException&) = default;
    FutureException& operator=(const FutureException&) = default;
    FutureException(FutureException&&) = default;
    FutureException& operator=(FutureException&&) = default;
    ~FutureException() override = default;

    const char* what() const noexcept override { return FutureException::from_type(_type); }
    Type type() const noexcept {  return _type; }

private:
    static const char* from_type(Type type) noexcept {
        switch (type) {
        case Type::broken_promise: return "broken promise";
        case Type::already_retrived: return "already retrived";
        case Type::already_satisfied: return "already satisfied";
        case Type::not_ready: return "not ready";
        default: COREY_UNREACHABLE();
        }
    }

    Type _type;
};

class BrokenPromise: public FutureException {
public:
    BrokenPromise() : FutureException(Type::broken_promise) {}
};

class FutureAlreadyRetrived: public FutureException {
public:
    FutureAlreadyRetrived() : FutureException(Type::already_retrived) {}
};


class PromiseAlreadySatisfied: public FutureException {
public:
    PromiseAlreadySatisfied() : FutureException(Type::already_satisfied) {}
};

class FutureNotReady: public FutureException {
public:
    FutureNotReady() : FutureException(Type::not_ready) {}
};

template<typename Data>
inline constexpr std::size_t DataExceptionEnumSize = std::max(sizeof(Data), sizeof(std::exception_ptr));

template<>
inline constexpr std::size_t DataExceptionEnumSize<void> = sizeof(std::exception_ptr);

template<typename Data>
inline constexpr std::size_t DataExceptionEnumAlign = std::max(alignof(Data), alignof(std::exception_ptr));

template<>
inline
constexpr std::size_t DataExceptionEnumAlign<void> = alignof(std::exception_ptr);

template <typename Data>
class State {
public:
    struct EmptyTag {};

    State(EmptyTag) noexcept : mode(Mode::empty), ref_cnt(0) { ; }

    State(std::exception_ptr ptr) noexcept : mode(Mode::exception), ref_cnt(0) {
        new(this->bytes.data()) std::exception_ptr(ptr);
    }

    template<typename... Args>
    State() requires (std::is_void_v<Data>) : mode(Mode::data), ref_cnt(0) { }

    template<typename... Args>
    State(Args&&... args) requires (!std::is_void_v<Data>) : mode(Mode::data), ref_cnt(0) {
        new(this->bytes.data()) Data(std::forward<Args>(args)...);
    }

    State(const State<Data>&) = delete;
    State& operator=(const State<Data>&) = delete;

    State(State<Data>&& other) noexcept : mode(other.mode), ref_cnt(other.ref_cnt) {
        switch(mode) {
        case Mode::empty:
            break;
        case Mode::data:
            if constexpr (!std::is_void_v<Data>) {
                new(this->bytes.data()) Data(std::move(*reinterpret_cast<Data*>(other.bytes.data())));
            }
            other.mode = Mode::empty;
            other.ref_cnt = 0;
            break;
        case Mode::exception:
            new(this->bytes.data()) std::exception_ptr(*reinterpret_cast<std::exception_ptr*>(other.bytes.data()));
            other.mode = Mode::empty;
            other.ref_cnt = 0;
            break;
        default:
            panic("Unkown state: {}", to_underlying(this->mode));
        }
    }

    State& operator=(State<Data>&& rhs) noexcept {
        if (this != &rhs) {
            this->~State();
            new(this) State(std::move(rhs));
        }
        return *this;
    }

    void get() requires std::is_void_v<Data> {
        switch(this->mode) {
        case Mode::empty:
            throw FutureNotReady();
        case Mode::data:
            return;
        case Mode::exception:
            std::rethrow_exception(*reinterpret_cast<std::exception_ptr*>(this->bytes.data()));
        default:
            panic("Unkown state: {}",to_underlying(this->mode));
        }
    }

    [[nodiscard]]
    auto& get() requires (!std::is_void_v<Data>) {
        switch(this->mode) {
        case Mode::empty:
            throw FutureNotReady();
        case Mode::data:
            return *reinterpret_cast<Data*>(this->bytes.data());
        case Mode::exception:
            std::rethrow_exception(*reinterpret_cast<std::exception_ptr*>(this->bytes.data()));
        default:
            panic("Unkown state: {}",to_underlying(this->mode));
        }
    }

    [[nodiscard]]
    std::exception_ptr get_exception() noexcept {
        switch(this->mode) {
        case Mode::empty: return std::make_exception_ptr(FutureNotReady());
        case Mode::data: return {};
        case Mode::exception: return *reinterpret_cast<std::exception_ptr*>(this->bytes.data());
        default: panic("Unkown state: {}",to_underlying(this->mode));
        }
    }

    [[nodiscard]]
    bool is_ready() noexcept { return this->mode != Mode::empty; }

    [[nodiscard]]
    bool has_failed() noexcept { return this->mode == Mode::exception; }

    template<typename... Args>
    void set(Args&&... args) {
        static_assert((std::is_void_v<Data> && (sizeof...(Args) == 0)) || (!std::is_void_v<Data>), "can't set data for void");
        if constexpr (!std::is_void_v<Data>) {
            new(this->bytes.data()) Data(std::forward<Args>(args)...);
        }
        this->mode = Mode::data;
    }

    void set_exception(std::exception_ptr ptr) {
        new(this->bytes.data()) std::exception_ptr(ptr);
        this->mode = Mode::exception;
    }

    void clear() {
        this->~State();
        this->mode = Mode::empty;
    }

    [[nodiscard]]
    std::uint32_t get_ref_cnt() const noexcept {
        return this->ref_cnt;
    }

private:

    ~State() {
        switch(this->mode) {
        case Mode::empty: break;
        case Mode::data:
            if constexpr (!std::is_void_v<Data>) {
                reinterpret_cast<Data*>(this->bytes.data())->~Data();
            }
            break;
        case Mode::exception: reinterpret_cast<std::exception_ptr*>(this->bytes.data())->~exception_ptr(); break;
        default:
            panic("Unkown state: {}", to_underlying(this->mode));
        }
    }

    friend void intrusive_ptr_add_ref(State<Data>* ptr) noexcept {
        ++ptr->ref_cnt;
    }
    friend void intrusive_ptr_release(State<Data>* ptr) noexcept {
        if (ptr->ref_cnt == 0) {
            panic("Invalid release!");
        }
        if ((--ptr->ref_cnt) == 0){ 
            delete ptr;
        }
    }

    enum class Mode: std::uint32_t {
        empty,
        data,
        exception
    } mode;
    std::uint32_t ref_cnt;

    alignas(DataExceptionEnumAlign<Data>)
    std::array<uint8_t, DataExceptionEnumSize<Data>> bytes;
};

template <typename Data = void>
class Promise;

template <typename Data = void>
class [[nodiscard]] Future {
    friend class Promise<Data>;
public:

    using PromiseType = Promise<Data>;

    Future() = delete;
    Future(const Future<Data>&) = delete;
    Future& operator=(const Future<Data>&) = delete;

    Future(Future<Data>&& other) : state(std::move(other.state)) {}

    Future& operator=(Future<Data>&& rhs) {
        if (this->state != rhs.state) {
            this->state = std::move(rhs.state);
        }
        return *this;
    }

    ~Future() = default;

    auto get() { return this->state->get(); }

    [[nodiscard]]
    auto get_exception() noexcept { return this->state->get_exception(); }

    [[nodiscard]]
    bool is_ready() const noexcept { return this->state->is_ready(); }

    [[nodiscard]]
    bool has_failed() const noexcept { return this->state->has_failed(); }

    template<typename DataFut, typename... Args>
    friend Future<DataFut> make_ready_future(Args&&... args);

    template<typename DataFut>
    friend Future<DataFut> make_exception_future(std::exception_ptr);

private:

    Future(const IntrusivePtr<State<Data>>& state) noexcept : state(state) { ; }
    Future(IntrusivePtr<State<Data>>&& state) noexcept : state(std::move(state)) { ; }

    IntrusivePtr<State<Data>> state;
};

template <typename Data>
class Promise {
public:

    using FutureType = Future<Data>;

    Promise() noexcept = default;

    Promise(const Promise<Data>&) = delete;
    Promise& operator=(const Promise<Data>&) = delete;

    Promise(Promise<Data>&& other) noexcept : state(std::move(other.state)) { ; }

    Promise& operator=(Promise<Data>&& rhs) noexcept {
        if (this != &rhs) {
            this->~Promise();
            this->state = std::move(rhs.state);
        }
        return *this;
    }

    ~Promise() {
        if ((this->state) && (this->state->get_ref_cnt() > 1) && (!this->state->is_ready())) {
            set_exception(std::make_exception_ptr(BrokenPromise()));
        }
    }

    [[nodiscard]]
    FutureType get_future() {
        if (!this->state) {
            this->state = IntrusivePtr<State<Data>>(new State<Data>(typename State<Data>::EmptyTag()));
        }
        if (this->state->get_ref_cnt() > 1) {
            throw FutureAlreadyRetrived();
        }
        return FutureType(this->state);
    }

    template<typename... Args>
    void set(Args&&... args) {
        if (!this->state) {
            this->state = IntrusivePtr<State<Data>>(new State<Data>(std::forward<Args>(args)...));
        } else {
            if (this->state->is_ready()) {
                throw PromiseAlreadySatisfied();
            }
            this->state->set(std::forward<Args>(args)...);
        }
    }

    void set_exception(std::exception_ptr exc) {
        if (!exc) {
            throw std::invalid_argument("exception_ptr is nullptr");
        }
        if (!this->state) {
            this->state = IntrusivePtr<State<Data>>(new State<Data>(exc));
        } else {
            if (this->state->is_ready()) {
                throw PromiseAlreadySatisfied();
            }
            this->state->set_exception(exc);
        }
    }

    template<typename Arg>
    void set_exception(Arg&& arg) = delete;

private:
    IntrusivePtr<State<Data>> state;
};

template<typename DataFut, typename... Args>
Future<DataFut> make_ready_future(Args&&... args) {
    return Future<DataFut>(IntrusivePtr<State<DataFut>>(new State<DataFut>(std::forward<Args>(args)...)));
}

template<typename DataFut>
Future<DataFut> make_exception_future(std::exception_ptr exc) {
    return Future<DataFut>(IntrusivePtr<State<DataFut>>(new State<DataFut>(std::move(exc))));
}

} // namespace gdc
