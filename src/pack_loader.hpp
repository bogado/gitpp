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
#include "pack_index.hpp"

namespace git {

class object_description : public index_item {
    std::string type;
    size_t size;
    size_t pack_size;
    size_t pack_depth;
    std::string pack_parent;
public:
    object_description(
        const std::string& name_ = "",
        const std::string& type_ = "",
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

    auto get_type() const {
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

class pack_loader : public file_loader  {
    pack_index index;
public:

};

}

#endif
