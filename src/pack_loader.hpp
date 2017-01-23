#ifndef PACK_LOADER_HPP_INCLUDED
#define PACK_LOADER_HPP_INCLUDED

#include <vector>
#include <iterator>
#include <algorithm>
#include <exception>
#include <iostream>
#include <fstream>
#include <string>
#include <utility>

#include "filesystem.hpp"
#include "pack_index.hpp"
#include "big_unsigned.hpp"

namespace git {

constexpr auto PACK_FILE_EXTENSION = ".pack";

class object_description : public index_item {
    const std::string& type;
    size_t size;
    size_t pack_size;
    size_t pack_depth;
    std::string pack_parent;
public:

    object_description(
        const std::string& name_,
        const std::string& type_,
        size_t size_ = 0,
        size_t pack_size_ = 0,
        size_t pack_offset_ = 0,
        size_t pack_depth_ = 0,
        std::string pack_parent_ = ""
    ) :
        index_item(name_, pack_offset_),
        type{type_},
        size{size_},
        pack_size{pack_size_},
        pack_depth{pack_depth_},
        pack_parent{pack_parent_}
    {}

    const std::string& get_type() const {
        return type;
    }

    auto get_size() const {
        return size;
    }

    auto get_pack_size() {
        return pack_size;
    }

    auto get_pack_depth() {
        return pack_depth;
    }

    auto get_pack_parent() {
        return pack_parent;
    }
};


template <class STREAM, typename INDEX_T = size_t>
class pack_loader :
    public index_iterable<pack_loader<STREAM, INDEX_T>, INDEX_T>
{
    static constexpr auto INFORMATION_BITS = 3;

    using ITERABLE = index_iterable<pack_loader<STREAM, INDEX_T>, INDEX_T>;
    using index_parser_type = index_reader_base<STREAM, INDEX_T>;

    using git_big_unsigned = big_unsigned_base<uint8_t, INFORMATION_BITS>;

    enum git_internal_type {
        COMMIT = 1,
        TREE   = 2,
        BLOB   = 3,
        TAG    = 4,
        DELTA_WITH_OFFSET = 6,
        DELTA_WITH_OBJID  = 7
    };

    static git_internal_type make_type(const git_big_unsigned& size_type) {
        uint8_t type_id = size_type.get_reserved_bits();
        return static_cast<git_internal_type>(type_id);
    }

    static const std::string& get_type_name(git_internal_type type) {
        static const std::string TYPES[] = {
            "invalid",
            "commit",
            "tree",
            "blob",
            "tag"
        };

        uint8_t index = static_cast<uint8_t>(type);
        if (index > 4) {
            index = 0;
        }

        return TYPES[index];
    };

public:
    using stream_t = STREAM;
    using index_type = INDEX_T;
    using value_type = object_description;
    using reference  = typename ITERABLE::reference;
    using pointer    = typename ITERABLE::pointer;

private:
    index_parser_type index_parser;
    mutable stream_t pack_input;

    git_big_unsigned get_size_and_type(size_t offset) const {
        pack_input.seekg(offset, std::ios::beg);
        git_big_unsigned result;
        result.binread(pack_input);
        return result;
    }

public:
    template <typename... ARGS>
    pack_loader(index_parser_type&& index_parser_instance, ARGS&&... args) :
        ITERABLE(*this),
        index_parser(std::move(index_parser_instance)),
        pack_input(std::forward<ARGS>(args)...)
    {
    }

    index_type size() const {
        return index_parser.size();
    }

    auto operator[](index_type index) const {
        auto index_item = index_parser[index];
        auto size_type = get_size_and_type(index_item.get_pack_offset());

        return value_type{
            index_item.get_name(),
            get_type_name(make_type(size_type)), // TODO: read type.
            size_type.template convert<size_t>(),  // TODO: read size.
            0,  // TODO: read pack size.
            index_item.get_pack_offset(),
            0,  // TODO: offset.
            ""  // TODO: parent.
        };
    }
};

template <class PATH>
auto pack_file_parser(const PATH& pack_base_path) {
    return pack_loader<std::ifstream>(
            index_file_parser(pack_base_path + INDEX_FILE_EXTENSION),
            pack_base_path + PACK_FILE_EXTENSION);
}

}

#endif
