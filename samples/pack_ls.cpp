#include "pack_loader.hpp"
#include "filesystem.hpp"

#include "find_pack.hpp"

#include <iostream>
#include <iomanip>

using namespace std;
using namespace git;
using namespace git::fs;

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

int main(int argc, const char* argv[]) {
    path pack;
    if (argc == 1) {
        pack = current_path();
    } else {
        pack = argv[1];
    }

    if (!find_pack(pack)) {
        return -1;
    }

    auto pack_loader = pack_file_parser(pack);

    ls_pack(pack_loader);
}

