#ifndef PACK_LOADER_HPP_INCLUDED
#define PACK_LOADER_HPP_INCLUDED

#include <vector>
#include <iterator>
#include <algorithm>
#include <exception>

#include "filesystem.hpp"

namespace git {

class index_item {
public:
    auto name() const {
        return "";
    }

    auto type() const {
        return "";
    }

    size_t size() const {
        return 0;
    }

    size_t pack_size() {
        return 0;
    }

    size_t pack_offset() {
        return 0;
    }

    size_t pack_depth() {
        return 0;
    }

    auto pack_parent() {
        return "";
    }
};

template <class STREAM, typename INDEX = int>
class index_reader_base {
    using stream_t = STREAM;
    using index_t = INDEX;

    stream_t &input;

    std::array<index_t, 256> summary;

    int get_next(int i) {}
    int get_prev(int i) {}

    void init() {
        static constexpr char V2_HEADER[] = { -1, 116, 79, 99};

        static std::vector<index_item> result;
        std::array<char, sizeof(uint32_t)> buffer;

        input.read(buffer.data(), buffer.size());
        if (!std::equal(buffer.begin(), buffer.end(), V2_HEADER)) {
            std::stringstream err;
            err << "V1 index is not supported [ ";
            for (auto &val : buffer) { err << unsigned(uint8_t(val)) << " "; }
            err << "]";
            throw std::invalid_argument(err.str());
        }
    }

    void load_summary() {
        static const size_t size = 4;
        index_t acc;
        char buffer[size];

        for (auto& entry: summary) {
            input.read(buffer, size);
            entry = 0;
            for (auto& byte: buffer) {
                entry <<= 8;
                entry += static_cast<uint8_t>(byte);
            }
        }
    }

public:
    template <bool isConst = false>
    class iterator_base {
        using container_ref = typename std::conditional<isConst,
                 const index_reader_base&,
                 index_reader_base&>::type;
        index_t index;
        container_ref reader;
    public:

        iterator_base(index_t start, container_ref ref) :
            index(start),
            reader(ref) {}

        auto operator++() {
            index++;
            return this;
        }

        auto operator++(int) {
            auto result = *this;
            index++;
            return result;
        }

        auto operator--() {
            index--;
            return this;
        }

        auto operator--(int) {
            auto result = *this;
            index--;
            return result;
        }

        auto operator*() const {
            return reader[index];
        }

        bool operator==(const iterator_base& other) const {
            return index == other.index && (&reader == &other.reader);
        }

        bool operator!=(const iterator_base& other) const {
            return !(*this == other);
        }
    };

    using value_type = index_item;
    using reference = index_item&;
    using pointer = index_item*;
    using iterator = iterator_base<false>;
    using const_iterator = iterator_base<true>;


    iterator begin() {
        return iterator {0, *this};
    }

    iterator end() {
        return iterator {size(), *this};
    }

    const_iterator cbegin() const {
        return const_iterator {0, *this};
    }

    const_iterator cend() const {
        return const_iterator {size(), *this};
    }

    auto operator[](int index) {
        return value_type{};
    }

    index_reader_base(stream_t& in) :
        input(in)
    {
        init();
        load_summary();
    }

    index_t size() const {
        return summary.back();
    }

    template <class OUT>
    void dump(OUT& dump_stream) const {
        dump_stream << "[ ";
        int i = 0;
        for(auto& val: summary) {
            dump_stream << std::hex << (i++) << ":" << val << " ";
        }
        dump_stream << "]";
    }
};

template <class INT, class BUFFER=std::array<std::uint8_t, sizeof(INT)> >
INT from_buffer(const BUFFER& buffer) {
    INT result = 0;
    for (auto value : buffer) {
        result <<= 8;
        result += value;
    }
    return result;
}

template <class STREAM>
auto index_file_parser(STREAM& in,
         typename std::enable_if<
            std::is_base_of<std::istream, STREAM>::value >::type* = 0) {
    return index_reader_base<STREAM>{in};
}

template <class PATH>
auto index_file_parser(PATH filename,
         typename std::enable_if<
            !std::is_base_of<std::istream, PATH>::value >::type* = 0) {
    git::fs::ifstream stream(filename);
    return index_file_parser(stream);
}

}

#endif
