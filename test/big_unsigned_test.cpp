#include "big_unsigned.hpp"

#include <sstream>

#include <bandit/bandit.h>

using namespace bandit;
using namespace git;

void big_unsigned_test() {
    const static std::uint8_t num_32[] = {
        0x20
    };

    const static std::uint8_t num_255[] = {
        0x81, 0x7f
    };

    // 0000'0100 0000'0001 => 1000'1000 0000'0001
    const static std::uint8_t num_1025[] = {
        0x88, 0x01
    };
    // 1 00000000 00000000 = 1000'0100 1000'0000 0000'0000
    const static std::uint8_t num_65536[] = {
        0x84, 0x80, 0x00
    };

    describe("big_unsigned tests", [&]() {

        std::stringstream stream;
        before_each([&]() {
            stream = std::stringstream{};
        });
        big_unsigned test;

        auto load = [&](const auto& arr) -> std::string {
                std::string source(std::begin(arr), std::end(arr));
                stream.str(source);
                test.binread(stream);
                return source;
        };

        auto load_and_convert_test = [&](const auto& arr, auto value) {
            it((std::string("Check binary I/O and convert for ") + std::to_string(value)).c_str(), [&]() {
                std::string source = load(arr);

                AssertThat(test.convert<decltype(value)>(), Equals(value));

                std::stringstream out;
                test.binwrite(out);
                AssertThat(out.str(), Is().EqualToContainer(source));
            });
        };

        auto load_and_print_test = [&](const auto& arr, auto print) {
            it((std::string("print test for ") + print).c_str(), [&]() {
                load(arr);

                std::stringstream out;
                out << test;
                AssertThat(out.str(), Equals(print));
            });
        };

        load_and_convert_test(num_32, uint8_t(32));
        load_and_print_test(num_32, "0x20");
        load_and_convert_test(num_255, uint8_t(255));
        load_and_print_test(num_255, "0xff");
        load_and_convert_test(num_1025, uint16_t(1025));
        load_and_print_test(num_1025, "0x401");
        load_and_convert_test(num_65536, uint32_t(65536));
        load_and_print_test(num_65536, "0x10000");

        auto convert_all_directions = [&](auto value) {
            test = value;
            AssertThat(test.convert<decltype(value)>(), Equals(value));
        };

        it("Convertion test", [&]() {
            for (unsigned i=0; i < 0x1000; i++) {
                convert_all_directions(i * 7);
                convert_all_directions(i * 0x1000);
                convert_all_directions(i * 0x1000000);
            }
        });
    });
}
