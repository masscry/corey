#pragma once

#include <memory>
#include <functional>
#include <optional>
#include <span>
#include <string>

namespace corey {

class Sink {
    struct IDevice {
        virtual ~IDevice() = default;
        virtual int write(std::span<const char> data) const = 0;
        virtual const void* get() const = 0;
    };
    using SPDevice = std::shared_ptr<IDevice>;
public:

    Sink() = default;
    Sink(const Sink& other) = default;
    Sink& operator=(const Sink& other) = default;
    Sink(Sink&&) noexcept = default;
    Sink& operator=(Sink&&) noexcept = default;
    ~Sink() = default;

    template<typename Device, typename... Args>
    static Sink make(Args&&... args) {
        struct DeviceImpl : public IDevice {
            DeviceImpl(Args&&... args) : _dev(std::forward<Args>(args)...) {}
            int write(std::span<const char> data) const override { return _dev.write(data); }
            const void* get() const override { return &_dev; }
            Device _dev;
        };
        return Sink(std::make_shared<DeviceImpl>(std::forward<Args>(args)...));
    }

    int write(std::span<const char> data) const {
        return _dev->write(data);
    }

    template<typename Device>
    const Device& as() const {        
        return *static_cast<const Device*>(_dev->get());
    }

    friend
    bool operator==(const Sink& lhs, const Sink& rhs) {
        return lhs._dev == rhs._dev;
    }

private:
    Sink(SPDevice&& dev) noexcept : _dev(std::move(dev)) {}

    SPDevice _dev;
};

} // namespace corey
