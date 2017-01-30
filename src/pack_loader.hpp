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
#include <tuple>

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
    index_item delta_parent;
public:

    object_description(
        const std::string& name_,
        const std::string& type_,
        size_t size_ = 0,
        size_t pack_size_ = 0,
        size_t pack_offset_ = 0,
        size_t pack_depth_ = 0,
        index_item parent = index_item{}
    ) :
        index_item(name_, pack_offset_),
        type{type_},
        size{size_},
        pack_size{pack_size_},
        pack_depth{pack_depth_},
        delta_parent{parent}
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

    auto get_delta_parent() {
        return delta_parent;
    }
};


template <class STREAM, typename INDEX_T = size_t>
class pack_loader :
    public index_iterable<pack_loader<STREAM, INDEX_T>, INDEX_T>
{
    using ITERABLE = index_iterable<pack_loader<STREAM, INDEX_T>, INDEX_T>;
    using index_parser_type = index_reader_base<STREAM, INDEX_T>;

    enum git_internal_type {
        COMMIT = 1,
        TREE   = 2,
        BLOB   = 3,
        TAG    = 4,
        DELTA_WITH_OFFSET = 6,
        DELTA_WITH_OBJID  = 7
    };

    static bool is_delta(git_internal_type type) {
        return type == DELTA_WITH_OFFSET || type == DELTA_WITH_OBJID;
    }

    static const std::string& type_name(git_internal_type type) {
        static const std::string TYPES[] = {
            "invalid",
            "commit",
            "tree",
            "blob",
            "tag",
            "invalid(5)",
            "delta by offset",
            "delta by object id"
        };

        uint8_t index = static_cast<uint8_t>(type);
        return TYPES[index];
    }

    const std::string& get_type_name(git_internal_type type, const index_item& parent) const {
        if (is_delta(type)) {
            size_t size;
            git_internal_type parent_type;
            index_item grand_parent;

            std::tie(size, parent_type, grand_parent) = load_data(parent);

            return get_type_name(parent_type, grand_parent);
        }
        return type_name(type);
    }

public:
    using stream_t = STREAM;
    using index_type = INDEX_T;
    using value_type = object_description;
    using reference  = typename ITERABLE::reference;
    using pointer    = typename ITERABLE::pointer;

private:
    index_parser_type index_parser;
    mutable stream_t pack_input;

    auto load_data(const index_item& item) const {
        item.seek_stream(pack_input);

        big_unsigned_with_type result;
        result.binread(pack_input);

        uint8_t type_id = result.get_reserved_bits();
        auto type = static_cast<git_internal_type>(type_id);
        size_t size = result.template convert<size_t>();

        index_item parent;
        if (is_delta(type)) {
            if (type == DELTA_WITH_OFFSET) {
                big_unsigned offset;
                offset.binread(pack_input);
                parent = item - offset.template convert<size_t>();
            } else {
                std::string parent_name = read_name_from(pack_input);
                parent = index_parser[parent_name];
                if (!parent) {
                    throw "broke!";
                }
            }
        }

        return std::make_tuple(size, type, parent);
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

    template <typename ITEM_ID>
    auto operator[](ITEM_ID index) const {
        auto index_found = index_parser[index];
        size_t size;
        git_internal_type type;
        index_item parent;

        std::tie(size, type, parent) = load_data(index_found);

        return value_type{
            index_found.get_name(),
            get_type_name(type, parent), // TODO: read type.
            size,  // TODO: read size.
            0,  // TODO: read pack size.
            index_found.get_pack_offset(),
            0, // TODO: offset.
            parent
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
