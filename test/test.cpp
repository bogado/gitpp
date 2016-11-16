#include <bandit/bandit.h>

#include <string>
#include <sstream>
#include <vector>

#include "big_unsigned.hpp"

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
#include "big_unsigned_test.hpp"

int main(int argc, char* argv[]) {
    return bandit::run(argc, argv);
}
