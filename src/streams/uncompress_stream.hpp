#ifndef UNCOMPRESS_SOURCE_HPP_INCLUDED
#define UNCOMPRESS_SOURCE_HPP_INCLUDED

#include "sources.hpp"

#include <iostream>

#include <boost/iostreams/restrict.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/zlib.hpp>

namespace git {

template <class DEVICE>
class uncompressed_source : device_source<DEVICE> {
    using parent_t = device_source<DEVICE>;
    uint64_t uncompress_size;
protected:
    using parent_t::get_source_device;

public:
    uncompressed_source(uint64_t size_, parent_t&& raw_source) :
        device_source<DEVICE>(std::move(raw_source)),
        uncompress_size(size_)
    {}

    uint64_t size() {
        return uncompress_size;
    }

    auto stream() const {
        auto result = std::make_unique<boost::iostreams::filtering_istream>();

        boost::iostreams::zlib_decompressor decompress_filter{};
        result->push(decompress_filter);

        result->push(get_source_device());

        result->set_auto_close(false);

        return result;
    }
};

template <typename SOURCE>
auto make_uncompressed_source(uint64_t size, SOURCE&& source) {
    return uncompressed_source<typename std::remove_reference<SOURCE>::type::device_t>{size, std::move(source)};
}

}

#endif
