#ifndef FIND_PACK_HPP_INCLUDED
#define FIND_PACK_HPP_INCLUDED

#include <algorithm>

#include "util/filesystem.hpp"

inline bool is_pack(git::fs::path file) {
    return file.extension() == ".pack" || file.extension() == ".idx";
}

bool find_pack(git::fs::path& pack) {
    while (! is_pack(pack)) {
        auto directory = git::fs::directory_iterator(pack);
        auto found = std::find_if(begin(directory), end(directory), is_pack);
        if (found == end(directory)) {
            std::cout << pack.string() << " : could not find git pack\n";
            return false;
        }
        pack = *found;
    }
    return true;
}
#endif
