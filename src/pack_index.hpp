#ifndef PACK_INDEX_HPP_INCLUDED
#define PACK_INDEX_HPP_INCLUDED

#include "buffer.hpp"
#include "indexed_iterator.hpp"
#include "iohelper.hpp"

#include <vector>
#include <sstream>
#include <utility>
#include <algorithm>
#include <map>

namespace git {

constexpr auto INDEX_FILE_EXTENSION = ".idx";
constexpr auto GIT_OBJECT_NAME_SIZE = 20;

class index_item {
    std::string name;
    uint64_t pack_offset;
    uint32_t crc;
public:

    explicit index_item(
        const std::string& name_ = "",
        uint64_t offset_ = 0,
        uint32_t crc_ = 0
    ) :
        name(name_),
        pack_offset(offset_),
        crc(crc_)
    {}

    explicit index_item(size_t offset_) :
        index_item{"", offset_}
    {}

    index_item operator-(size_t diff) const {
        if (diff == 0) {
            return *this;
        }
        return index_item{ pack_offset - diff };
    }

    auto get_name() const {
        return name;
    }

    auto get_pack_offset() const {
        return pack_offset;
    }

    auto get_crc() const {
        return crc;
    }

    operator bool() const {
        return pack_offset != 0;
    }

    void seek_stream(std::ostream& input) const {
        if (!*this) {
            return;
        }
        input.clear();
        input.seekp(pack_offset, std::ios_base::beg);
    }

    void seek_stream(std::istream& input) const {
        if (!*this) {
            return;
        }
        input.clear();
        input.seekg(pack_offset, std::ios_base::beg);
    }
};

namespace {

inline char hex_digit (unsigned v) {
    v &= 0x0f;
    if (v < 10) {
        return '0' + v;
    } else {
        return 'a' + (v - 10);
    }
}

inline uint8_t hex_value(char d) {
    if (d >= 'a' && d <= 'f') { return d - 'a' + 10; }
    if (d >= 'A' && d <= 'F') { return d - 'A' + 10; }
    if (d >= '0' && d <= '9') { return d - '0'; }
    return 0;
}

inline std::string read_name_from(std::istream& input) {
    char buffer[GIT_OBJECT_NAME_SIZE];
    input.read(buffer, GIT_OBJECT_NAME_SIZE);

    std::string result(GIT_OBJECT_NAME_SIZE * 2, ' ');
    auto it = result.begin();
    for (char v: buffer) {
        *it = hex_digit(v >> 4);
        it++;
        *it = hex_digit(v & 0x0f);
        it++;
    }

    return result;
}

}

// TODO: no support for > 4gb packages
template <class STREAM, typename INDEX_T = size_t>
class index_reader_base :
    public index_iterable<index_reader_base<STREAM, INDEX_T>, INDEX_T>
{
    using ITERABLE = index_iterable<index_reader_base<STREAM, INDEX_T>, INDEX_T>;

public:
    using index_type = INDEX_T;
    using value_type = index_item;
    using reference  = typename ITERABLE::reference;
    using pointer    = typename ITERABLE::pointer;

    using ITERABLE::begin;
    using ITERABLE::end;

private:
    using stream_t = STREAM;

    mutable stream_t input;

    static const index_type SUMMARY_OFFSET = 8;
    static const index_type SUMMARY_ENTRY_SIZE = 4;
    static const index_type SUMMARY_SIZE = 256 * SUMMARY_ENTRY_SIZE;

    static const index_type NAMES_OFFSET = SUMMARY_OFFSET + SUMMARY_SIZE;
    static const index_type NAME_SIZE = GIT_OBJECT_NAME_SIZE;

    static const index_type CRC_SIZE = 4;
    static const index_type OFFSET_SIZE = 4;

    std::array<index_type, 256> summary;
    std::map<size_t, index_type> offset_index;

    index_type start_crcs = 0;
    index_type start_offsets = 0;

    void init() {
        static constexpr char V2_HEADER[] = { -1, 116, 79, 99};

        std::array<char, sizeof(uint32_t)> buffer;

        input.read(buffer.data(), buffer.size());
        if (!std::equal(buffer.begin(), buffer.end(), V2_HEADER)) {
            std::stringstream err;
            err << "V1 index is not supported (yet) [ ";
            for (auto &val : buffer) { err << +val << " "; }
            err << "]";
            throw std::invalid_argument(err.str());
        }
    }

    void load_summary() {
        static const size_t size = 4;
        char buffer[size];

        input.seekg(SUMMARY_OFFSET, std::ios::beg);

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
        return read_name_from(input);
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

    void load_offset_index() {
        for(index_type i = 0; i < size(); i++) {
            offset_index.emplace(read_offset(i), i);
        }
    }

public:
    template <typename... ARGS>
    index_reader_base(ARGS && ... args) :
        ITERABLE(*this),
        input(std::forward<ARGS>(args)...)
    {
        init();
        load_summary();
        start_crcs = name_location(size());
        start_offsets = crc_location(size());
        load_offset_index();
    }

    auto operator[](index_type index) const {
        value_type result;
        if (index < size()) {
            result = value_type {
                read_name(index),
                read_offset(index),
                read_crc(index)
            };
        }
        return result;
    }

    auto operator[](const value_type& item) const {
        auto found = offset_index.find(item.get_pack_offset());
        if (found == offset_index.end()) {
            return value_type{};
        }
        return (*this)[found->second];
    }

    auto operator[](std::string name) const {
        uint8_t first = hex_value(name[0]) * 16 + hex_value(name[1]);

        auto start = begin() + (first > 0 ? summary[first-1] : 0);
        auto finish = start + (summary[first] - summary[first-1]);

        auto found = std::lower_bound(start, finish, value_type{name, 0}, [](const auto &a, const auto& b) {
            return a.get_name() < b.get_name();
        });
        if (found == end() || (*found).get_name() != name) {
            return index_item{};
        } else {
            return *found;
        }
    }

    auto next(const value_type& item) const {
        auto offset = item.get_pack_offset();
        auto it = offset_index.find(offset);
        if (it != offset_index.end()) {
            std::advance(it, 1);
            return (*this)[it->second];
        } else {
            return value_type{};
        }
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
