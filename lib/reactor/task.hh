#pragma once

#include <utility>
#include <memory>
#include <span>

namespace corey {

class Task {
    struct Model {
        Model() = default;
        Model(const Model &) = default;
        Model(Model &&) noexcept = default;
        Model &operator=(const Model &) = default;
        Model &operator=(Model &&) noexcept = default;
        [[nodiscard]] virtual std::unique_ptr<Model> clone() const = 0;
        virtual void execute() const = 0;
        virtual ~Model() = 0;
    };
    using UPModel = std::unique_ptr<Model>;
public:

    Task() : _model(nullptr) { ; }
    Task(const Task& other) : _model(other._model?other._model->clone():UPModel()) {;}
    Task(Task&& other) noexcept : _model(std::move(other._model)) { other._model = nullptr; }
    Task& operator=(const Task& other) {
        if (this != &other) {
            auto copy = Task(other);
            this->~Task();
            new (this) Task(std::move(copy));
        }
        return *this;
    }
    Task& operator=(Task&& other) noexcept {
        if (this != &other) {
            this->~Task();
            new (this) Task(std::move(other));
        }
        return *this;
    }
    ~Task() = default;

    void execute() const {
        return _model->execute();
    }

    template<typename Impl, typename... Args>
    static
    Task make(Args&&... args) {
        struct ConcreteModel: public Model {
            ConcreteModel(Args&&... args) : _task(std::forward<Args>(args)...) {}
            ConcreteModel(const ConcreteModel &) = default;
            ConcreteModel(ConcreteModel &&) = delete;
            ConcreteModel &operator=(const ConcreteModel &) = default;
            ConcreteModel &operator=(ConcreteModel &&) = delete;
            [[nodiscard]] UPModel clone() const override { return std::make_unique<ConcreteModel>(*this); }; 
            void execute() const override { return _task.execute(); }
            ~ConcreteModel() override = default;
        private:
            Impl _task;
        };
        return Task(std::make_unique<ConcreteModel>(std::forward<Args>(args)...));
    }

private:
    explicit Task(UPModel&& model) noexcept : _model(std::move(model)) {}
    UPModel _model;
};

template<typename Func>
Task make_task(Func&& func) {
    struct Lambda {
        Func _func;
        Lambda(Func&& func) : _func(std::forward<Func>(func)) {}
        void execute() const { _func(); }
    };
    return Task::make<Lambda>(std::forward<Func>(func));
}

} // namespace corey
