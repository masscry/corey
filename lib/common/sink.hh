#pragma once

#include <memory>
#include <functional>
#include <optional>
#include <span>

namespace corey {

template<typename Impl = void>
class Sink {
    friend class Sink<void>;
public:
    Sink(const Sink<Impl>&) = delete;
    Sink& operator=(const Sink<Impl>&) = delete;
    Sink& operator=(Sink<Impl>&& other) noexcept = default;
    Sink(Sink<Impl>&& other) noexcept = default;
    ~Sink() = default;

    int write(std::span<char> data) const {
        return _impl.write(data);
    }

    template<typename... Args>
    static Sink<Impl> make(Args&&... args) {
        return Sink<Impl>(std::forward<Args>(args)...);
    }

    const Impl& get_impl() const noexcept { return _impl; }

private:
    template<typename... Args>
    explicit Sink(Args&&... args) noexcept : _impl(std::forward<Args>(args)...) { ; }
    Impl _impl;
};

template<>
class Sink<void> {
    struct Model {
        virtual int write(std::span<char> data) const = 0;
        virtual ~Model() = default;
    };

    template<typename Impl>
    struct SinkModel: public Model {
        int write(std::span<char> data) const override { return _sink.write(data); }

        template<typename... Args>
        SinkModel(Args&&... args) : _sink(std::forward<Args>(args)...) { ; }
        ~SinkModel() override = default;
        Sink<Impl>& get() noexcept { return _sink; }
    private:
        Sink<Impl> _sink;
    };

    using UPModel = std::unique_ptr<Model>;
public:

    Sink() = default;
    Sink(const Sink<void>&) = delete;
    Sink& operator=(const Sink<void>&) = delete;
    Sink& operator=(Sink<void>&& other) noexcept = default;
    Sink(Sink<void>&& other) noexcept = default;
    ~Sink() = default;

    template<typename Impl>
    Sink(Sink<Impl>&& other) noexcept : _model(std::make_unique<SinkModel<Impl>>(std::move(other))) { ; }

    template<typename Impl>
    Sink& operator=(Sink<Impl>&& other) noexcept {
        if (this != &other) {
            this->~Sink();
            new (this) Sink(std::make_unique<SinkModel<Impl>>(std::move(other)));
        }
        return *this;
    }

    template<typename Impl>
    const Sink<Impl>& as() const {
        return static_cast<SinkModel<Impl>*>(_model.get())->get();
    }

    int write(std::span<char> data) const {
        return _model->write(data);
    }

    template<typename Impl, typename... Args>
    static Sink<void> make(Args&&... args) {
        return Sink<void>(std::unique_ptr<SinkModel<Impl>>(
            new SinkModel<Impl>(std::forward<Args>(args)...))
        );
    }

private:
    explicit Sink(UPModel&& model) noexcept : _model(std::move(model)) { ; }
    UPModel _model;
};


} // namespace corey
