#include "pack/loader.hpp"
#include "util/filesystem.hpp"

#include <iostream>
#include <iterator>

#include "find_pack.hpp"

using namespace std;
using namespace git;
using namespace git::fs;

template <typename T>
void cat_object(T& pack_loader, const std::string& name, std::string path) {
    auto& object = pack_loader[name];

    if (!object) {
        std::cout << "Could not find object '" << name << "' at " << path << "\n";
        return;
    }
    auto& stream = object.get_stream();
    stream.unsetf(ios_base::skipws);
    auto begin = istream_iterator<char>(stream);
    auto end   = istream_iterator<char>();
    if (begin == end) {
        std::cout << "Empty object found.\n";
        return;
    }
    copy(begin, end, ostream_iterator<char>(std::cout, ""));
}

int main(int argc, const char* argv[]) {
    if (argc < 2) {
        cout << "Usage: " << argv[0] << " (<git pack file>) <Object ID>";
    }

    string obj;
    path pack;
    if (argc == 3) {
        pack = argv[1];
        obj = argv[2];
    } else {
        pack = current_path();
        obj = argv[1];
    }

    if (!find_pack(pack)) {
        return -1;
    }

    auto pack_loader = pack_file_parser(pack);

    cat_object(pack_loader, argv[2], pack.string());
}

