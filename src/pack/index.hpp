#ifndef PACK_INDEX_HPP_INCLUDED
#define PACK_INDEX_HPP_INCLUDED

#include "util/buffer.hpp"
#include "util/indexed_iterator.hpp"
#include "util/filesystem.hpp"
#include "util/git_definitions.hpp"
#include "streams/iohelper.hpp"
#include "streams/sources.hpp"

#include <vector>
#include <sstream>
#include <utility>
#include <algorithm>
#include <map>
#include <iostream>

namespace git {

constexpr auto INDEX_FILE_EXTENSION = ".idx";

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

    const std::string& get_name() const {
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
};


/** git pack index loader.
 *
 * This class reads a pack index and retrieves the positioning details for each object.
 *
 * Helper class to implement the pack object repository.
 */
// TODO: no support for > 4gb packages
template <class SOURCE, typename INDEX_T = size_t>
class index_reader_base :
    public index_iterable<index_reader_base<SOURCE, INDEX_T>, INDEX_T>
{
    using ITERABLE = index_iterable<index_reader_base<SOURCE, INDEX_T>, INDEX_T>;

public:
    using index_type = INDEX_T;
    using value_type = index_item;
    using reference  = typename ITERABLE::reference;
    using pointer    = typename ITERABLE::pointer;

    using ITERABLE::begin;
    using ITERABLE::end;

private:
    using source_t = SOURCE;

    mutable source_t index_source;

    static const index_type SUMMARY_OFFSET = 8;
    static const index_type SUMMARY_ENTRY_SIZE = 4;
    static const index_type SUMMARY_SIZE = 256 * SUMMARY_ENTRY_SIZE;

    static const index_type NAMES_OFFSET = SUMMARY_OFFSET + SUMMARY_SIZE;
    static const index_type NAME_SIZE = OBJECT_NAME_SIZE;

    static const index_type CRC_SIZE = 4;
    static const index_type OFFSET_SIZE = 4;

    std::array<index_type, 256> summary;
    std::map<size_t, index_type> offset_index;

    index_type start_crcs = 0;
    index_type start_offsets = 0;

    void init() {
        static constexpr char V2_HEADER[] = { -1, 116, 79, 99};

        std::array<char, sizeof(uint32_t)> buffer;

        auto input = index_source.stream();

        input->read(buffer.data(), buffer.size());
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

        auto input = index_source.substream(SUMMARY_OFFSET);

        for (auto& entry: summary) {
            input->read(buffer, size);
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
        return read_object_name_from(*index_source.substream(name_location(index)));
    }

    auto crc_location(unsigned index) const {
        return start_crcs + index * CRC_SIZE;
    }

    auto read_crc(unsigned index) const {
        return read_netorder_at<uint32_t>(index_source, crc_location(index));
    }

    auto offset_location(unsigned index) const {
        return start_offsets + index * OFFSET_SIZE;
    }

    auto read_offset(unsigned index) const {
        return read_netorder_at<uint32_t>(index_source, offset_location(index));
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
        index_source(std::forward<ARGS>(args)...)
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
        if (name == "") return value_type{};

        int first, second;

        if (name.size() >= 2) {
            second = hex_value(name[0]) * 16 + hex_value(name[1]);
            first = second - 1;
        } else {
            first = hex_value(name[0]) * 16 ;
            second = first + 15;
        }

        auto start = first > 0 ? summary[first] : 0;
        auto finish = start + summary[second] - start;

        auto value = value_type{name, 0};
        auto range = std::equal_range(
                begin() + start,
                begin() + finish,
                value,
                [](const auto &a, const auto& b) {
                    auto& name_a = a.get_name();
                    auto& name_b = b.get_name();

                    auto size = std::min(name_a.size(), name_b.size());
                    if (size == 0) return false;
                    return lexicographical_compare(
                            std::begin(name_a), std::begin(name_a) + size,
                            std::begin(name_b), std::begin(name_b) + size);
                }
        );

         if (range.first != end() && std::distance(range.first, range.second) == 1) {
            return *range.first;
        } else {
            return index_item{};
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

template <typename PATH>
static fs::path get_index_path(PATH file) {
    fs::path file_path{file};
    file_path.replace_extension(INDEX_FILE_EXTENSION);
    return file_path;
}

template <class PATH>
auto index_file_parser(PATH filename) {
    return index_reader_base<file_source>(get_index_path(filename).string());
}

}

#endif
