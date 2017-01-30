#include "big_unsigned.hpp"

#include <sstream>

#include <bandit/bandit.h>

using namespace bandit;
using namespace git;

void big_unsigned_test() {
    const static std::uint8_t num_32[] = {
        0x20
    };

    const static std::uint8_t num_32_NG[] = {
        0x20
    };

    const static std::uint8_t num_255[] = {
        0x80, 0x7f
    };

    const static std::uint8_t num_255_NG[] = {
        0x81, 0x7f
    };

    // 0000'0100 0000'0001 => 1000'0111 0000'0001
    const static std::uint8_t num_1025[] = {
        0x87, 0x01
    };

    // 0000'0100 0000'0001 => 1000'1000 0000'0001
    const static std::uint8_t num_1025_NG[] = {
        0x88, 0x01
    };

    // 1 00000000 00000000 = 1000'0011 1111'1111 0000'0000
    const static std::uint8_t num_65536[] = {
        0x82, 0xff, 0x00
    };

    // 1 00000000 00000000 = 1000'0100 1000'0000 0000'0000
    const static std::uint8_t num_65536_NG[] = {
        0x84, 0x80, 0x00
    };
/* 0001=1 0010=2 0011=3 0100=4
 * 0101=5 0110=6 0111=7 1000=8
 * 1001=9 1010=a 1011=b 1100=c
 * 1101=d 1110=e 1111=f 0000=0
 */
    // 0001 0010 0011 0100 0101 0110 0111 1000 1001 1010 1011 1100 1101 1110 1111
    //
    // 1 000 0001
    // 1 001 0001
    // 1 101 0001
    // 1 010 1100
    // 1 111 1000
    // 1 100 1101
    // 1 010 1111
    // 1 001 1011
    // 0 110 1111
    const static std::uint8_t num_0x123456789abcdef_NG[] = {
        0x81, 0x91, 0xd1, 0xac, 0xf8, 0xcd, 0xaf, 0x9b, 0x6f
    };

    const static std::uint8_t num_0x123456789abcdef[] = {
        0x80, 0x90, 0xd0, 0xab, 0xf7, 0xcc, 0xae, 0x9a, 0x6f
    };

    describe("big_unsigned tests", [&]() {

        std::stringstream stream;
        before_each([&]() {
            stream = std::stringstream{};
        });

        auto load = [&](const auto& arr, auto &test) -> std::string {
                std::string source(std::begin(arr), std::end(arr));
                stream.str(source);
                auto expected_size = std::distance(std::begin(arr), std::end(arr));
                AssertThat(stream.str().size(), Equals(expected_size));
                test.binread(stream);
                AssertThat(test.size(), Equals(expected_size));
                return source;
        };

        auto load_and_convert_test = [&](const auto& arr, auto value, auto test) {
            std::stringstream desc;
            desc << "Check binary I/O and convert for " << value << " with " << (sizeof(value) * 8) << " bits";
            auto description = desc.str();
            it(description.c_str(), [&]() {

                decltype(test) converted{value};
                AssertThat(converted, Equals(test));

                decltype(test) loaded;
                std::string source = load(arr, loaded);
                AssertThat(loaded.template convert<decltype(value)>(), Equals(value));
                AssertThat(loaded, Equals(test));

                AssertThat(converted, Equals(loaded));

                std::stringstream out;
                test.binwrite(out);

                AssertThat(out.str(), EqualsContainer(source));
            });
        };

        auto load_and_print_test = [&](const auto& arr, auto print, auto test) {
            it((std::string("print test for ") + print).c_str(), [&]() {
                decltype(test) loaded;
                load(arr, loaded);

                std::stringstream out;
                out << loaded;
                AssertThat(out.str(), Equals(print));
            });
        };

        auto runTests = [&](const auto& source, auto value, auto print, auto test) {
            load_and_convert_test(source, value, test);
            load_and_print_test(source, print, test);
        };

        describe("run tests with non-git mode", [&]() {
            runTests(num_32_NG,   uint8_t(32),     "0x20",    big_unsigned_non_git{32});
            runTests(num_255_NG,  uint8_t(255),    "0xff",    big_unsigned_non_git{255});
            runTests(num_1025_NG, uint16_t(1025),  "0x401",   big_unsigned_non_git{1025});
            runTests(num_65536_NG,uint32_t(65536), "0x10000", big_unsigned_non_git{65536});

            runTests(num_32_NG,   uint64_t(32),    "0x20",    big_unsigned_non_git{32});
            runTests(num_255_NG,  uint64_t(255),   "0xff",    big_unsigned_non_git{255});
            runTests(num_1025_NG, uint64_t(1025),  "0x401",   big_unsigned_non_git{1025});
            runTests(num_65536_NG,uint64_t(65536), "0x10000", big_unsigned_non_git{65536});

            runTests(                 num_0x123456789abcdef_NG,
                                 uint64_t(0x123456789abcdef),
                                         "0x123456789abcdef",
                     big_unsigned_non_git{0x123456789abcdef});

        });

        describe("run tests with git mode", [&]() {
            runTests(num_32,   uint8_t(32),     "0x20",    big_unsigned{32});
            runTests(num_255,  uint8_t(255),    "0xff",    big_unsigned{255});
            runTests(num_1025, uint16_t(1025),  "0x401",   big_unsigned{1025});
            runTests(num_65536,uint32_t(65536), "0x10000", big_unsigned{65536});

            runTests(num_32,   uint64_t(32),    "0x20",    big_unsigned{32});
            runTests(num_255,  uint64_t(255),   "0xff",    big_unsigned{255});
            runTests(num_1025, uint64_t(1025),  "0x401",   big_unsigned{1025});
            runTests(num_65536,uint64_t(65536), "0x10000", big_unsigned{65536});

            runTests(         num_0x123456789abcdef,
                         uint64_t(0x123456789abcdef),
                                 "0x123456789abcdef",
                     big_unsigned{0x123456789abcdef});
        });

        auto convert_all_directions = [&](auto value, auto test) {
            test = value;
            AssertThat(test.template convert<decltype(value)>(), Equals(value));
        };

        it("Convertion test", [&]() {
            for (unsigned i=0; i < 0x1000; i++) {
                convert_all_directions(i * 7,             big_unsigned{});
                convert_all_directions(i * 0x7000,        big_unsigned{});
                convert_all_directions(i * 0x7000000,     big_unsigned{});
                convert_all_directions(i * 0x56789abcdef, big_unsigned{});
                convert_all_directions(i * 7,             big_unsigned_non_git{});
                convert_all_directions(i * 0x7000,        big_unsigned_non_git{});
                convert_all_directions(i * 0x7000000,     big_unsigned_non_git{});
                convert_all_directions(i * 0x7000000,     big_unsigned_non_git{});
                convert_all_directions(i * 0x56789abcdef, big_unsigned_non_git{});
            }
        });
    });
}
