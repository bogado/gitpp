#define TESTS_READY
#include "pack_loader_test.hpp"
#include "big_unsigned_test.hpp"

#include <bandit/bandit.h>

#include <string>
#include <sstream>
#include <vector>

using namespace git;
using namespace bandit;

// Those are helper functions :

namespace snowhouse {

template <>
struct Stringizer<std::string> {
    static std::string ToString(const std::string& v) {
        std::stringstream out;
        out << "[ " << std::hex;
        for (unsigned char c : v) {
            out << "0x" << unsigned(c) << " ";
        }
        out << "] \"" << v << "\"";
        return out.str();
    }
};

}

#define TESTS_READY

void big_unsigned_test();
void pack_data_test();

go_bandit([]{
    big_unsigned_test();
    pack_data_test();
});

int main(int argc, char* argv[]) {
    return bandit::run(argc, argv);
}
