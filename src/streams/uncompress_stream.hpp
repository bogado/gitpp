#ifndef UNCOMPRESS_SOURCE_HPP_INCLUDED
#define UNCOMPRESS_SOURCE_HPP_INCLUDED

#include "sources.hpp"

#include <iostream>

#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/restrict.hpp>

namespace git {

template <typename DEVICE_T>
class uncompressed_device : public device {
    DEVICE_T decompressed_source;

public:
    using device_type = DEVICE_T;

    std::unique_ptr<std::streambuf> create_limited_buffer(size_t start, std::optional<size_t> length) override {
        namespace io = boost::iostreams;

        io::stream_offset sz = -1;
        if (length) {
            sz = *length;
        }

        std::stringstream uncompress_data;

        std::unique_ptr<io::filtering_streambuf<io::input>> out;
        out->push(io::restrict(io::zlib_decompressor{}, start, sz));
        out->push(*decompressed_source.create_buffer().release());

        return out;
    }

    uncompressed_device(device_type raw_source) :
        decompressed_source{std::move(raw_source)}
    {}

    std::optional<size_t> size() const override {
        return std::optional<size_t>(); // Unknown size.
    }
};

template <typename DEVICE>
auto make_uncompressed_source(DEVICE source) {
    return device_source<uncompressed_device<DEVICE>>{source};
}

}

#endif
