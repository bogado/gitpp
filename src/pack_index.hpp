#ifndef PACK_INDEX_HPP_INCLUDED
#define PACK_INDEX_HPP_INCLUDED

#include "buffer.hpp"
#include "indexed_iterator.hpp"
#include "iohelper.hpp"

#include <vector>
#include <sstream>
#include <utility>
#include <algorithm>

namespace git {

constexpr auto INDEX_FILE_EXTENSION = ".idx";
constexpr auto GIT_OBJECT_NAME_SIZE = 20;

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

    operator bool() const {
        return pack_offset != 0;
    }

    // TODO: different implementations of this index item for each type of offset.
    template <typename STREAM>
    void seek_stream(STREAM& input) const {
        if (!*this) {
            return;
        }
        input.clear();
        input.seekg(pack_offset, std::ios_base::beg);
    }
};

namespace {
char hex_digit (unsigned v) {
    v &= 0x0f;
    if (v < 10) {
        return '0' + v;
    } else {
        return 'a' + (v - 10);
    }
}
}

template <class STREAM>
std::string read_name_from(STREAM& input) {
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
    }

    auto operator[](index_type index) const {
        value_type result;
        if (index < size()) {
            result = value_type {
                read_name(index),
                read_offset(index)
            };
        }
        return result;
    }

    auto operator[](std::string name) const {
        auto found = std::lower_bound(begin(), end(), value_type{name, 0}, [](const auto &a, const auto& b) {
            return a.get_name() < b.get_name();
        });
        if (found == end() || (*found).get_name() != name) {
            return index_item{};
        } else {
            return *found;
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
