#ifndef SOURCES_HPP_INCLUDED
#define SOURCES_HPP_INCLUDED

#include "util/shared_container.hpp"

#include <istream>
#include <streambuf>
#include <memory>
#include <optional>

#include "util/filesystem.hpp"

namespace git {

/** A device is a factory for streambufs.
 *
 * Each buffer have to hold a shared reference to the device.
 * As such devices can only be created as shared pointers for this reason
 * the constructor is protected and a static template make_shared creates
 * a derived class.
 */
class device : public std::enable_shared_from_this<device> {
public:
    using shared_ptr = std::shared_ptr<device>;

    template <class DERIVED, class... ARG_TYPES>
    static shared_ptr make_shared(ARG_TYPES&&... args) {
        static_assert(std::is_base_of<device, DERIVED>::value, "DERIVED must override device");
        return std::make_shared<DERIVED>(std::forward<ARG_TYPES>(args)...);
    }

    virtual ~device() {}

    std::unique_ptr<std::streambuf> create_buffer() {
        return create_limited_buffer(0, size());
    }

    std::unique_ptr<std::streambuf> create_sub_buffer(size_t start, size_t length) {
        return create_limited_buffer(start, length);
    }

    std::unique_ptr<std::streambuf> create_sub_buffer(size_t start) {
        auto sz = size();
        if (sz) {
            sz.value() -= start;
        }
        return create_limited_buffer(start, sz);
    }

    virtual std::optional<size_t> size() const = 0;

    virtual std::unique_ptr<std::streambuf> create_limited_buffer(size_t start, std::optional<size_t> length) = 0;

protected:
    device() {}
};

/** A device source is responsible for building streams for devices.
 *
 */
template <typename T,
         typename = std::enable_if_t<std::is_base_of_v<device, T>>>
class device_source : public device {
    std::shared_ptr<T> my_device;
    size_t start = 0;
    std::optional<size_t> extent;

    auto subclone(size_t start, std::optional<size_t> size) const {
        auto result = *this;
        result.start += start;
        result.extent = size;
        return result;
    }

protected:
    device::shared_ptr& get_source_device() {
        return my_device;
    }

    class device_istream : public std::istream {
        std::unique_ptr<std::streambuf> buffer;
    public:
        device_istream(std::unique_ptr<std::streambuf>&& buffer) :
            std::istream(buffer.get()),
            buffer(std::move(buffer))
        {}
    };

public:
    template <typename... ARGS>
    device_source(ARGS... arg) :
        my_device(std::make_unique<T>(std::forward<ARGS>(arg)...)),
        extent(my_device->size())
    {}

    std::unique_ptr<std::istream> stream() const {
        return std::make_unique<device_istream>(my_device->create_sub_buffer(start));
    }

    std::unique_ptr<std::istream> substream(size_t offset, size_t size) const {
        return std::make_unique<device_istream>(my_device->create_sub_buffer(start+offset, size));
    }

    std::unique_ptr<std::istream> substream(size_t offset) const {
        return std::make_unique<device_istream>(my_device->create_sub_buffer(start+offset));
    }

    device_source subsource(size_t start, size_t size) const {
        return subclone(start, size);
    }

    device_source subsource(size_t start) const {
        if (extent) {
            return subclone(start, *extent - start);
        } else {
            return subclone(start, extent);
        }
    }

    std::optional<size_t> size() const override {
        return extent;
    }

    std::unique_ptr<std::streambuf> create_limited_buffer(size_t st, std::optional<size_t> length) override {
        if (!length) {
            length = extent;
        }

        return my_device->create_limited_buffer(start+st, length);
    }
};

}

#endif
