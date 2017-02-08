#include "pack_loader.hpp"
#include "filesystem.hpp"

using namespace std;
using namespace git;
using namespace git::fs;

inline bool is_pack(path file) {
    return file.extension() == ".pack" || file.extension() == ".idx";
}

void execute(path pack_file) {
    auto pack_loader = pack_file_parser(pack_file);
    for (const auto& obj: pack_loader) {
        cout << obj.get_name() << "(" << obj.get_type() << ") : @" <<
            obj.get_pack_offset() << " " << obj.get_size() << " bytes " <<
            obj.get_pack_size() << "bytes (packed)";

        const auto* delta = dynamic_cast<const pack_delta_descriptor*>(&obj);
        if (delta) {
            cout << " has " << delta->get_pack_depth() << " level(s) parent: " << delta->get_delta_parent().get_name();
        }
        cout << "\n";
    }
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
        pack = *found;
    }

    std::cout << "list of objects in " << pack << "\n";

    execute(pack);
}

