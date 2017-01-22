#ifndef PACK_INDEX_HPP_INCLUDED
#define PACK_INDEX_HPP_INCLUDED

#include "buffer.hpp"

#include <vector>
#include <sstream>
#include <utility>

namespace git {

class index_item {
    std::string name;
    size_t pack_offset;
public:

    index_item(
        const std::string& name_ = "",
        size_t offset_ = 0
    ) :
        name(name_),
        pack_offset(offset_)
    {}

    auto get_name() const {
        return name;
    }

    auto get_pack_offset() {
        return pack_offset;
    }

};

template <class SOURCE>
class indexed_iterator {
public:
    using index_type = typename SOURCE::index_type;
    using value_type = typename SOURCE::value_type;
    using reference = value_type&;
    using pointer   = value_type*;

private:
    index_type index;
    SOURCE& source;

public:
    indexed_iterator(index_type start, SOURCE& ret) :
        index(start),
        source(ret) {}

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
        return source[index];
    }

    bool operator==(const indexed_iterator& other) const {
        return &source == &other.source && index == other.index;
    }

    bool operator!=(const indexed_iterator& other) const {
        return !(*this == other);
    }

    index_type current_index() {
        return index;
    }
};

namespace {
    template <typename UNSIGNED_TYPE, typename STREAM>
    auto read_netorder_at(STREAM& input, typename STREAM::pos_type position)
        -> typename std::enable_if<std::is_unsigned<UNSIGNED_TYPE>::value, UNSIGNED_TYPE>::type
    {
        input.seekg(position, std::ios_base::beg);
        UNSIGNED_TYPE result = 0;
        uint8_t buffer[sizeof(UNSIGNED_TYPE)];
        input.read(reinterpret_cast<char*>(buffer), sizeof(buffer));

        return utils::from_buffer<UNSIGNED_TYPE>(buffer);
    }
}

template <class SOURCE,
         class VALUE_TYPE,
         class ITERATOR = indexed_iterator<SOURCE>>
class index_iterable {
    SOURCE& base;
public:
    index_iterable(SOURCE& b) :
        base(b)
    {}

    using value_type = VALUE_TYPE;
    using reference = value_type&;
    using pointer = value_type*;
    using iterator = ITERATOR;

    iterator begin() const {
        return iterator {0, base};
    }

    iterator end() const {
        return iterator {base.size(), base};
    }
};

// TODO: no support for > 4gb packages
template <class STREAM, typename INDEX_T = size_t>
class index_reader_base :
    public index_iterable<index_reader_base<STREAM, INDEX_T>, index_item>
{
    using ITERABLE = index_iterable<index_reader_base<STREAM, INDEX_T>, index_item>;

public:
    using index_type = INDEX_T;
    using value_type = index_item;
    using reference  = typename ITERABLE::reference;
    using pointer    = typename ITERABLE::pointer;


private:
    using stream_t = STREAM;

    mutable stream_t input;

    static const index_type SUMMARY_OFFSET = 8;
    static const index_type SUMMARY_ENTRY_SIZE = 4;
    static const index_type SUMMARY_SIZE = 256 * SUMMARY_ENTRY_SIZE;

    static const index_type NAMES_OFFSET = SUMMARY_OFFSET + SUMMARY_SIZE;
    static const index_type NAME_SIZE = 20;

    static const index_type CRC_SIZE = 4;
    static const index_type OFFSET_SIZE = 4;

    std::array<index_type, 256> summary;
    index_type start_crcs = 0;
    index_type start_offsets = 0;

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
        index_type acc;
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

    static constexpr index_type name_location(unsigned index) {
        return NAMES_OFFSET + NAME_SIZE * index;
    }

    std::string read_name(index_type index) const {
        input.clear();
        input.seekg(name_location(index), std::ios::beg);

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

    auto crc_location(unsigned index) const {
        return start_crcs + index * CRC_SIZE;
    }

    auto read_crc(unsigned index) const {
        return read_netorder_at<uint32_t>(input, crc_location(index));
    }

    auto offset_location(unsigned index) const {
        return start_offsets + index * OFFSET_SIZE;
    }

    auto read_offset(unsigned index) const {
        return read_netorder_at<uint32_t>(input, offset_location(index));
    }

public:
    auto operator[](index_type index) const {
        return value_type{
            read_name(index),
            read_offset(index)
        };
    }

    template <typename... ARGS>
    index_reader_base(ARGS && ... args) :
        ITERABLE(*this),
        input(std::forward<ARGS>(args)...)
    {
        init();
        load_summary();
        start_crcs = name_location(size());
        start_offsets = crc_location(size());
    }

    index_type size() const {
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

template <class PATH>
auto index_file_parser(PATH filename) {
    return index_reader_base<std::ifstream>(filename, std::ios::binary);
}

}

#endif
