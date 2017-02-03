#include <bandit/bandit.h>

#include <string>
#include <sstream>
#include <vector>

using namespace bandit;

// Those are helper functions :

namespace snowhouse {

template <>
struct Stringizer<std::string> {
    static std::string ToString(const std::string& string_value) {
        std::stringstream out;
        out << "[ " << std::hex;
        for (auto c : string_value) {
            out << "0x" << +c << " ";
        }
        out << "] \"" << string_value << "\"";
        return out.str();
    }
};

template<>
struct Stringizer<char> {
    static std::string ToSting(char v) {
        std::stringstream out;
        if (std::isprint(v)) {
            out << "'" << v <<"' ";
        }
        out << "(" << +v << ")";
        return out.str();
    }
};

}

void big_unsigned_test();
void pack_index_test();
void pack_data_test();

go_bandit([]{
    big_unsigned_test();
    pack_index_test();
    pack_data_test();
});

int main(int argc, char* argv[]) {
    return bandit::run(argc, argv);
}
