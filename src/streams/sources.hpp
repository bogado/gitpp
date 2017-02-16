#ifndef SOURCES_HPP_INCLUDED
#define SOURCES_HPP_INCLUDED

#include <iostream>
#include <memory>

#include "util/filesystem.hpp"

#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/iostreams/restrict.hpp>
#include <boost/iostreams/stream.hpp>

namespace git {

template <typename DEVICE>
class device_source {
    DEVICE device;

public:
    using device_t = DEVICE;

private:
    using stream_t   = boost::iostreams::stream<device_t>;
    using restrict_t = boost::iostreams::restriction<device_t>;

public:
    using subsource_t = device_source<restrict_t>;

protected:
    device_t& get_source_device() {
        return device;
    }

    const device_t& get_source_device() const {
        return device;
    }

public:
    template <class... ARGS>
    device_source(ARGS&&... args) :
        device(std::forward<ARGS>(args)...)
    {}

    std::unique_ptr<std::istream> stream() const {
        auto result = std::make_unique<stream_t>(device);
        result->set_auto_close(false);
        return std::move(result);
    }

    auto subsource(uint64_t offset, int64_t size = -1) const {
        return subsource_t{boost::iostreams::restrict(device, offset, size)};
    }

    auto substream(uint64_t offset, int64_t size = -1) const {
        return subsource(offset, size).stream();
    }
};

// mapped file source
class file_source : public device_source<boost::iostreams::mapped_file_source> {
    fs::path file_path;

public:
    file_source(fs::path path) :
        device_source{path.string()},
        file_path{path}
    {}

    uint64_t size() const {
        return get_source_device().size();
    }
};

}

#endif
