#include "big_unsigned.hpp"

#include <sstream>

#include <bandit/bandit.h>

using namespace bandit;
using namespace git;

namespace snowhouse {
    template <typename INT,
             unsigned reserved_bits,
             bool big_endian,
             class storage,
             unsigned value_bits>
    struct Stringizer<big_unsigned_base<INT, reserved_bits, big_endian, storage, value_bits> > {
        using type = big_unsigned_base<INT, reserved_bits, big_endian, storage, value_bits>;

        static std::string ToString(const type& value) {
            return value.dump();
        }
    };
}

void big_unsigned_test() {
    const static std::uint8_t num_32[] = {
        0x20
    };

    const static std::uint8_t num_32_WT[] = {
        0x80, 0x02
    };

    const static std::uint8_t num_255[] = {
        0x80, 0x7f
    };

    const static std::uint8_t num_255_BE[] = {
        0xff, 0x01
    };

    const static std::uint8_t num_255_WT[] = {
        0x8f, 0x0f
    };

    const static std::uint8_t num_1025[] = {
        0x87, 0x01
    };

    const static std::uint8_t num_1025_BE[] = {
        0x81, 0x08
    };

    const static std::uint8_t num_1025_WT[] = {
        0x81, 0x40
    };

    const static std::uint8_t num_65536[] = {
        0x82, 0xff, 0x00
    };

    const static std::uint8_t num_65536_BE[] = {
        0x80, 0x80, 0x04
    };

    const static std::uint8_t num_65536_WT[] = {
        0x80, 0x80, 0x20
    };

    const static std::uint8_t num_0x123456789abcdef[] = {
        0x80, 0x90, 0xd0, 0xab, 0xf7, 0xcc, 0xae, 0x9a, 0x6f
    };

    const static std::uint8_t num_0x123456789abcdef_BE[] = {
        0xef, 0x9b, 0xaf, 0xcd, 0xf8, 0xac, 0xd1, 0x91, 0x01
    };

    const static std::uint8_t num_0x123456789abcdef_WT[] = {
        0x8f, 0xde, 0xf9, 0xea, 0xc4, 0xe7, 0x8a, 0x8d, 0x09
    };

    describe("big_unsigned tests", [&]() {

        std::stringstream stream;
        before_each([&]() {
            stream = std::stringstream{};
        });

        auto tests = [&](const auto& arr, auto value, auto print, auto expected) {
            std::string source(std::begin(arr), std::end(arr));

            decltype(expected) loaded;
            it("load test", [&]() {
                stream.str(source);
                loaded.binread(stream);
                AssertThat(loaded, Equals(expected));
            });

            it("write to stream test", [&]() {
                std::stringstream out;
                expected.binwrite(out);
                AssertThat(out.str(), EqualsContainer(source));
            });

            decltype(expected) converted{value};
            it("convert to big_unsigned test", [&]() {
                AssertThat(converted, Equals(expected));
            });

            it("convert back to value test", [&]() {
                decltype(value) convert{expected.template convert<decltype(value)>()};
            });

            it("print test", [&]() {
                std::stringstream out;
                out << loaded;
                AssertThat(out.str(), Equals(print));
            });
        };

        auto runTests = [&](const auto& source, auto value, auto print, auto test) {
            std::stringstream desc;
            desc << test.dump() << " and " <<  print << "(" << +value << ")";
            auto description = desc.str();
            describe(description.c_str(), [&](){
                tests(source, value, print, test);
            });
        };

        runTests(num_32,      uint8_t (32),    "0x20",    big_unsigned_big_endian{32});
        runTests(num_255_BE,  uint8_t (255),   "0xff",    big_unsigned_big_endian{255});
        runTests(num_1025_BE, uint16_t(1025),  "0x401",   big_unsigned_big_endian{1025});
        runTests(num_65536_BE,uint32_t(65536), "0x10000", big_unsigned_big_endian{65536});

        runTests(num_32,      uint64_t(32),    "0x20",    big_unsigned_big_endian{32});
        runTests(num_255_BE,  uint64_t(255),   "0xff",    big_unsigned_big_endian{255});
        runTests(num_1025_BE, uint64_t(1025),  "0x401",   big_unsigned_big_endian{1025});
        runTests(num_65536_BE,uint64_t(65536), "0x10000", big_unsigned_big_endian{65536});

        runTests(                 num_0x123456789abcdef_BE,
                             uint64_t(0x123456789abcdef),
                                     "0x123456789abcdef",
                 big_unsigned_big_endian{0x123456789abcdef});

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

        runTests(num_32_WT,   uint8_t(32),     "0x20",    big_unsigned_with_type{32});
        runTests(num_255_WT,  uint8_t(255),    "0xff",    big_unsigned_with_type{255});
        runTests(num_1025_WT, uint16_t(1025),  "0x401",   big_unsigned_with_type{1025});
        runTests(num_65536_WT,uint32_t(65536), "0x10000", big_unsigned_with_type{65536});

        runTests(num_32_WT,   uint64_t(32),    "0x20",    big_unsigned_with_type{32});
        runTests(num_255_WT,  uint64_t(255),   "0xff",    big_unsigned_with_type{255});
        runTests(num_1025_WT, uint64_t(1025),  "0x401",   big_unsigned_with_type{1025});
        runTests(num_65536_WT,uint64_t(65536), "0x10000", big_unsigned_with_type{65536});

        runTests(         num_0x123456789abcdef_WT,
                     uint64_t(0x123456789abcdef),
                             "0x123456789abcdef",
                 big_unsigned_with_type{0x123456789abcdef});

        auto convert_all_directions = [&](auto value, auto test) {
            test = value;
            AssertThat(test.template convert<decltype(value)>(), Equals(value));
        };

        auto type_test = [&](auto value) {
            big_unsigned_with_type test{value};

            for (int val  = 0; val < (1 << GIT_TYPE_ENCODE_BITS); val++) {
                test.set_reserved_bits(val);
                AssertThat(test.get_reserved_bits(), Equals(val));
                AssertThat(test.template convert<decltype(value)>(), Equals(value));
            }
        };

        it("Back and forth conversion test", [&]() {
            for (unsigned i=0; i < 0x1000; i++) {
                convert_all_directions(i * 7,             big_unsigned{});
                convert_all_directions(i * 0x7000,        big_unsigned{});
                convert_all_directions(i * 0x7000000,     big_unsigned{});
                convert_all_directions(i * 0x56789abcdef, big_unsigned{});
                convert_all_directions(i * 7,             big_unsigned_big_endian{});
                convert_all_directions(i * 0x7000,        big_unsigned_big_endian{});
                convert_all_directions(i * 0x7000000,     big_unsigned_big_endian{});
                convert_all_directions(i * 0x7000000,     big_unsigned_big_endian{});
                convert_all_directions(i * 0x56789abcdef, big_unsigned_big_endian{});
                convert_all_directions(i * 7,             big_unsigned_with_type{});
                convert_all_directions(i * 0x7000,        big_unsigned_with_type{});
                convert_all_directions(i * 0x7000000,     big_unsigned_with_type{});
                convert_all_directions(i * 0x7000000,     big_unsigned_with_type{});
                convert_all_directions(i * 0x56789abcdef, big_unsigned_with_type{});
                type_test(i * 7);
                type_test(i * 0x7000);
                type_test(i * 0x7000000);
                type_test(i * 0x7000000);
                type_test(i * 0x56789abcdef);
            }
        });
    });
}
