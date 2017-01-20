#ifndef PACK_LOADER_HPP_INCLUDED
#define PACK_LOADER_HPP_INCLUDED

#include <vector>
#include <iterator>
#include <algorithm>
#include <exception>
#include <iostream>
#include <fstream>
#include <string>

#include "filesystem.hpp"

namespace git {

class index_item {
    std::string name;
    std::string type;
    size_t size;
    size_t pack_size;
    size_t pack_offset;
    size_t pack_depth;
    std::string pack_parent;
public:

    index_item(
        std::string name_ = "",
        std::string type_ = "",
        size_t size_ = 0,
        size_t pack_size_ = 0,
        size_t pack_offset_ = 0,
        size_t pack_depth_ = 0,
        std::string pack_parent_ = ""
    ) :
        name{name_},
        type{type_},
        size{size_},
        pack_size{pack_size_},
        pack_offset{pack_offset_},
        pack_depth{pack_depth_},
        pack_parent{pack_parent_}
    {}

    auto get_name() const {
        return name;
    }

    auto get_type() const {
        return type;
    }

    auto get_size() const {
        return size;
    }

    auto get_pack_size() {
        return pack_size;
    }

    auto get_pack_offset() {
        return pack_offset;
    }

    auto get_pack_depth() {
        return pack_depth;
    }

    auto get_pack_parent() {
        return pack_parent;
    }
};

template <class STREAM, typename INDEX = size_t>
class index_reader_base {
    using stream_t = STREAM;
    using index_t = INDEX;

    static const index_t SUMMARY_OFFSET = 8;
    static const index_t SUMMARY_SIZE = 256 * 4;
    static const index_t NAMES_OFFSET = SUMMARY_OFFSET + SUMMARY_SIZE;
    static const index_t NAME_SIZE = 20;

    mutable stream_t input;

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
            for (auto &val : buffer) { err << +val << " "; }
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

    std::string read_name(index_t i) const {
        input.clear();
        input.seekg(NAMES_OFFSET + 20 * i, std::ios::beg);

        char buffer[NAME_SIZE];
        input.read(buffer, NAME_SIZE);

        std::string result(NAME_SIZE * 2, ' ');
        auto it = result.begin();
        for (char v: buffer) {
            *it = digit(v >> 4);
            it++;
            *it = digit(v & 0x0f);
            it++;
        }

        return result;
    }

    static char digit (unsigned v) {
        v &= 0x0f;
        if (v < 10) {
            return '0' + v;
        } else {
            return 'a' + (v - 10);
        }
    }

public:
    class iterator {
        using container_ref = const index_reader_base&;
        index_t index;
        container_ref reader;
    public:

        iterator(index_t start, container_ref ref) :
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

        bool operator==(const iterator& other) const {
            return index == other.index && (&reader == &other.reader);
        }

        bool operator!=(const iterator& other) const {
            return !(*this == other);
        }
    };

    using value_type = index_item;
    using reference = index_item&;
    using pointer = index_item*;

    iterator begin() const {
        return iterator {0, *this};
    }

    iterator end() const {
        return iterator {size(), *this};
    }

    auto operator[](int index) const {
        return value_type{
            read_name(index)
        };
    }

    template <typename... ARGS>
    index_reader_base(ARGS... args) :
        input(args...)
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
        for (auto val: *this) {
            dump_stream << val.get_name() << "\n";
        }
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

template <class PATH>
auto index_file_parser(PATH filename) {
    return index_reader_base<std::ifstream>(filename, std::ios::binary);
}

}

#endif
