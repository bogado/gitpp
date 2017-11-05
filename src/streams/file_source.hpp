#ifndef FILE_SOURCE_HPP_INCLUDED
#define FILE_SOURCE_HPP_INCLUDED

#include "streams/sources.hpp"

#include <memory>
#include <functional>
#include <streambuf>
#include <iostream>

#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <system_error>

namespace git {

class file_descriptor {
    int fd;

public:
    file_descriptor(fs::path file) :
        fd{open(file.c_str(), O_RDONLY)}
    {}

    int reset() {
        int res = fd;
        fd = -1;
        return res;
    }

    void close() {
        if (fd != -1) {
            ::close(reset());
        }
    }

    operator int() {
        return fd;
    }

    ~file_descriptor() {
        close();
    }

    file_descriptor(file_descriptor&& f) {
        close();
        fd = f.fd;
    }

    file_descriptor& operator=(file_descriptor&& other) {
        close();
        fd = other.reset();
        return *this;
    }

    file_descriptor(file_descriptor&) = delete;
    file_descriptor& operator=(file_descriptor&) = delete;
};

template<typename CHAR_TYPE>
class file_mapper {
    size_t my_size;
    std::shared_ptr<CHAR_TYPE> mapped_memory;
public:
    using char_type = CHAR_TYPE;

    file_mapper(fs::path file) :
        my_size{fs::file_size(file)}
    {
        file_descriptor fd{file};
        auto sz = my_size;
        auto deleter = [sz](void *mem) {
            if (mem != MAP_FAILED) {
                munmap(mem, sz);
            }
        };

        mapped_memory.template reset<char_type, decltype(deleter) >(
            reinterpret_cast<char_type *>(mmap(nullptr, my_size, PROT_READ, MAP_SHARED, fd, 0)),
            deleter
        );
        if (mapped_memory.get() == MAP_FAILED) {
            throw std::system_error(errno, std::system_category());
        }
    }

    char_type operator*() const {
        return *mapped_memory.get();
    }

    char_type operator[](size_t offset) const {
        return mapped_memory.get()[offset];
    }

    char_type& operator*()  {
        return *mapped_memory.get();
    }

    char_type& operator[](size_t offset) {
        return mapped_memory.get()[offset];
    }

    auto get() const {
        return mapped_memory.get();
    }

    auto get() {
        return mapped_memory.get();
    }

    auto size() {
        return my_size;
    }
};

class mapped_file_buffer : public std::streambuf {
    using mapper_t = file_mapper<typename traits_type::char_type>;

    mapper_t mapped_file;
    char_type hold = 0;
    size_t start;
    size_t finish;
    size_t current;

public:
    mapped_file_buffer(mapper_t mfile, size_t offset, size_t len) :
        mapped_file{std::move(mfile)},
        start{offset},
        finish{offset + len},
        current{offset}
    {
        setg(&hold,  &hold, &hold);
    }

    mapped_file_buffer(mapper_t mfile, size_t offset) :
        mapped_file_buffer{std::move(mfile), offset, mfile.size() - offset}
    {}

    mapped_file_buffer(fs::path file) :
        mapped_file_buffer(file, 0, fs::file_size(file))
    {}

protected:
    int_type underflow() override
    {
        if (current == finish) {
            return traits_type::eof();
        }
        hold = mapped_file[current++];
        setg(&hold, &hold, &hold+1);
        return traits_type::to_int_type(hold);
    }

    pos_type seekpos(pos_type off, std::ios_base::openmode witch) override {
        if (witch == std::ios_base::out) {
            return 0;
        }
        current = off;
        return current - start;
    }

    pos_type seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode witch) override {
        if (witch == std::ios_base::out) {
            return 0;
        }
        current = (dir == std::ios_base::beg ? start : dir == std::ios_base::end ? finish : current) + off;
        return current - start;
    }


};

class file_device : public device {
    fs::path file_path;
    file_mapper<char> map;

public:
    file_device(fs::path path) :
        file_path{path},
        map{path}
    {}

    std::optional<size_t> size() const override {
        return file_size(file_path);
    }

    std::unique_ptr<std::streambuf> create_limited_buffer(size_t start, std::optional<size_t> length) override {
        size_t sz = length.value_or(start - size().value_or(0));
        return std::make_unique<mapped_file_buffer>(map, start, sz);
    }
};

using file_source = device_source<file_device>;

}

#endif
