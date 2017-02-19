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
#include <unordered_map>

#include "util/filesystem.hpp"

#include "pack/index.hpp"
#include "util/big_unsigned.hpp"
#include "streams/sources.hpp"
#include "streams/iohelper.hpp"
#include "streams/uncompress_stream.hpp"
#include "object_descriptor.hpp"


namespace git {

constexpr auto PACK_FILE_EXTENSION = ".pack";

/// Object description for packs.
class pack_object_descriptor : public index_item, public object_descriptor_base {
    unsigned header_size;
    size_t size;
    size_t pack_size;

protected:
    virtual unsigned get_header_size() const {
        return header_size;
    }

public:
    pack_object_descriptor(
        const std::string& name,
        unsigned header_size_ = 0,
        uint64_t size_ = 0,
        uint64_t pack_offset = 0,
        uint64_t pack_size_ = 0,
        uint32_t crc = 0
    ) :
        index_item{name, pack_offset, crc},
        header_size{header_size_},
        size{size_},
        pack_size{pack_size_}
    {}

    const std::string& get_name() const {
        return index_item::get_name();
    }

    virtual const std::string& get_type() const = 0;

    operator bool() const {
        return size > 0;
    }

    auto get_size() const {
        return size;
    }

    auto get_pack_size() const {
        return pack_size;
    }

    auto get_data_offset() const {
        return get_pack_offset() + get_header_size();
    }

    auto get_data_size() const {
        return static_cast<uint64_t>(get_pack_size() - get_header_size() + 1);
    }
};

class pack_delta_descriptor {
public:
    virtual ~pack_delta_descriptor() = default;

    virtual unsigned get_pack_depth() const = 0;
    virtual object_descriptor_base& get_delta_parent() const = 0;
};

template <class SOURCE, typename INDEX_T = size_t>
class pack_loader :
    public index_iterable<pack_loader<SOURCE, INDEX_T>, INDEX_T>
{
    static constexpr auto TAIL_SIZE = 20; // 20 bytes SHA1 checksum at the end of the file.

    using ITERABLE = index_iterable<pack_loader<SOURCE, INDEX_T>, INDEX_T>;
    using index_parser_type = index_reader_base<SOURCE, INDEX_T>;

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
            "invalid(0)",
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

public:
    using source_t = SOURCE;
    using index_type = INDEX_T;
    using value_type = pack_object_descriptor;
    using reference  = typename ITERABLE::reference;
    using pointer    = typename ITERABLE::pointer;

private:
    index_parser_type index_parser;

    using cache_t = std::unordered_map<std::string, std::unique_ptr<pack_object_descriptor>>;
    using cache_entry_t = cache_t::value_type;
    // Retrieve objects do no alter this.
    mutable source_t pack_source;
    mutable cache_t object_cache;

    class non_delta_object_descriptor : public pack_object_descriptor {
        git_internal_type type;
        const source_t& source;
        std::unique_ptr<std::istream> stream;

    public:
        non_delta_object_descriptor(
                const source_t& source_,
                const std::string& name,
                git_internal_type type_,
                unsigned header_size = 0,
                uint64_t size = 0,
                uint64_t pack_offset = 0,
                uint64_t pack_size = 0,
                uint32_t crc = 0) :
            pack_object_descriptor {
                name,
                header_size,
                size,
                pack_offset,
                pack_size,
                crc },
            type{type_},
            source{source_}
        {}

        const std::string& get_type() const override {
            return type_name(type);
        }

        std::istream& get_stream() override {
            if (!stream) {
                auto size = get_data_size();
                auto restricted_source = source.subsource(get_data_offset(), size);
                stream = make_uncompressed_source(size, std::move(restricted_source)).stream();
            }
            return *stream;
        }

        virtual bool has_parent() const {
            return false;
        }

        git_internal_type get_internal_type() const {
            return type;
        }
    };

    class delta_object_descriptor : public non_delta_object_descriptor, public pack_delta_descriptor {
        unsigned pack_depth = 0;
        unsigned extra_header = 0;
        non_delta_object_descriptor& parent;
        git_internal_type external_type;

        auto external_type_and_depth(unsigned depth = 0) const {
            if (!parent.has_parent()) {
                return std::make_pair(parent.get_internal_type(), depth + 1);
            }

            return dynamic_cast<delta_object_descriptor&>(parent).external_type_and_depth(depth + 1);
        }

    protected:
        bool has_parent() const override {
            return true;
        }

        unsigned get_header_size() const override {
            return non_delta_object_descriptor::get_header_size() + extra_header;
        }

    public:
        delta_object_descriptor(
                const source_t& source,
                object_descriptor_base& parent_,
                const std::string& name,
                git_internal_type type,
                unsigned header_size,
                uint64_t size = 0,
                uint64_t pack_offset = 0,
                uint64_t pack_size = 0,
                uint32_t crc = 0,
                unsigned extra_header_ = 0
            ) :
            non_delta_object_descriptor{
                source,
                name,
                type,
                header_size,
                size,
                pack_offset,
                pack_size,
                crc
            },
            extra_header{extra_header_},
            parent { dynamic_cast<non_delta_object_descriptor&>(parent_) }
        {
            std::tie(external_type, pack_depth) = external_type_and_depth();
        }

        unsigned get_pack_depth() const override {
            return pack_depth;
        }

        object_descriptor_base& get_delta_parent() const override {
            return parent;
        }

        const std::string& get_type() const override {
            return type_name(external_type);
        }
    };

    pack_object_descriptor& load_data(const index_item& item) const {
        auto it = object_cache.find(item.get_name());
        if (it == object_cache.end()) {
            auto pack_input = pack_source.subsource(item.get_pack_offset()).stream();

            big_unsigned_with_type result;
            result.binread(*pack_input);

            uint8_t type_id = result.get_reserved_bits();
            auto type = static_cast<git_internal_type>(type_id);
            size_t size = result.template convert<size_t>();

            if (is_delta(type)) {
                object_descriptor_base* parent;
                unsigned extra_header;
                if (type == DELTA_WITH_OFFSET) {
                    big_unsigned offset;
                    offset.binread(*pack_input);
                    extra_header = offset.size();
                    parent = &(*this)[item - offset.template convert<size_t>()];
                } else {
                    std::string parent_name = read_object_name_from(*pack_input);
                    extra_header = OBJECT_NAME_SIZE/2;
                    parent = &(*this)[parent_name];
                    if (!(parent && *parent)) {
                        throw "broke!";
                    }
                }

                cache_entry_t entry {
                        item.get_name(),
                        std::make_unique<delta_object_descriptor>(
                                pack_source,
                                *parent,
                                item.get_name(),
                                type,
                                result.size(),
                                size,
                                item.get_pack_offset(),
                                read_pack_size(item),
                                item.get_crc(),
                                extra_header
                        )};
                it = object_cache.insert(std::move(entry)).first;
            } else {
                it = object_cache.emplace(
                        item.get_name(),
                        std::make_unique<non_delta_object_descriptor>(
                            pack_source,
                            item.get_name(),
                            type,
                            result.size(),
                            size,
                            item.get_pack_offset(),
                            read_pack_size(item),
                            item.get_crc()
                        )
                ).first;
            }
        }

        return *it->second;
    }

    uint64_t read_pack_size(index_item index) const {
        if (!index) {
            return 0;
        }

        auto next_object = index_parser.next(index);
        if (next_object) {
            return next_object.get_pack_offset() - index.get_pack_offset();
        }

        return pack_source.size() - index.get_pack_offset() - TAIL_SIZE;
    }

public:
    template <typename... ARGS>
    pack_loader(index_parser_type&& index_parser_instance, ARGS&&... args) :
        ITERABLE(*this),
        index_parser(std::move(index_parser_instance)),
        pack_source(std::forward<ARGS>(args)...)
    {}

    index_type size() const {
        return index_parser.size();
    }

    template <typename ITEM_ID>
    auto& operator[](ITEM_ID index) const {
        auto index_found = index_parser[index];

        return load_data(index_found);
    }
};

template <typename PATH>
fs::path get_pack_path(PATH file) {
    fs::path file_path{file};
    file_path.replace_extension(PACK_FILE_EXTENSION);
    return file_path;
}

template <class PATH>
auto pack_file_parser(const PATH& pack_base_path) {
    return pack_loader<file_source>(
            index_file_parser(pack_base_path),
            get_pack_path(pack_base_path));
}

}

#endif
