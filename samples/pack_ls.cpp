#include "pack_loader.hpp"
#include "filesystem.hpp"

#include <iterator>
#include <iostream>
#include <iomanip>

using namespace std;
using namespace git;
using namespace git::fs;

inline bool is_pack(path file) {
    return file.extension() == ".pack" || file.extension() == ".idx";
}

template <typename T>
void ls_pack(T& pack_loader) {
    for (const auto& obj: pack_loader) {
        cout << obj.get_name()
            << "(" << setw(6) << setfill(' ') << obj.get_type() << ") : @" <<
            hex << showbase << setw(8) << setfill('0') << internal << obj.get_pack_offset() << " " <<
            dec << setw(6) << setfill(' ') << obj.get_size() << " bytes " <<
            dec << setw(6) << setfill(' ') << obj.get_pack_size() << " bytes (packed) " <<
            "data [ " << hex << setfill('0') << setw(8) << obj.get_data_offset() << " - " << setw(8) << (obj.get_data_offset() + obj.get_data_size()) << " ]" << dec << obj.get_data_size();

        const auto* delta = dynamic_cast<const pack_delta_descriptor*>(&obj);
        if (delta) {
            cout << "\ndelta has " << delta->get_pack_depth() << " level(s) parent: " << delta->get_delta_parent().get_name() << "\n";
        }
        cout << "\n";
    }
}

template <typename T>
void cat_object(T& pack_loader, const std::string& name) {
    auto& object = pack_loader[name];
    auto begin = std::istream_iterator<char>(object.get_stream());
    auto end   = std::istream_iterator<char>();
    std::copy(begin, end, std::ostream_iterator<char>(std::cout, ""));
}

int main(int argc, const char* argv[]) {
    path pack;
    if (argc <= 1) {
        pack = current_path();
    } else {
        pack = argv[1];
    }

    while (! is_pack(pack)) {
        auto directory = directory_iterator(pack);
        auto found = find_if(begin(directory), end(directory), [&](const auto& file) -> bool {
            return is_pack(file);
        });
        if (found == end(directory)) {
            std::cout << pack.string() << " : could not find git pack\n";
            return -1;
        }
        pack = *found;
    }

    auto pack_loader = pack_file_parser(pack);

    if (argc == 3) {
        cat_object(pack_loader, argv[2]);
    } else {
        std::cout << "list of objects in " << pack << "\n";

        ls_pack(pack_loader);
    }
}

