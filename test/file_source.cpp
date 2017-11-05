#include "streams/file_source.hpp"

#include "object_tests.hpp"

#include <bandit/bandit.h>
#include <string>

using namespace git;
using namespace bandit;
using namespace snowhouse;

const static std::string SAMPLE_TEST_FILE{TEST_RESOURCE_PATH "/test.txt"};

void file_source_test() {
    describe("map file", [&]() {
        it("read contents", []() {
            file_mapper<unsigned char> mapped{SAMPLE_TEST_FILE};
            AssertThat(mapped[0], Equals('1'));
            AssertThat(mapped[1], Equals('2'));
            AssertThat(mapped[2], Equals('3'));
            AssertThat(mapped[3], Equals('4'));
        });
    });

    auto fdev = device_source<file_device>(SAMPLE_TEST_FILE);

    describe("stream test", [&]() {
        it("file_device_read", [&]() {
            auto stream = fdev.stream();
            auto stream2 = fdev.stream();
            std::string str;
            int val;
            *stream2 >> str;
            AssertThat(str, Equals(std::string("1234")));
            *stream >> val;
            AssertThat(val, Equals(1234));
        });

        it("file_device_seek", [&]() {
            auto stream = fdev.stream();

            auto test_read = [&](auto expected) mutable {
                std::vector<char> buffer(expected.size());
                stream->read(buffer.data(), buffer.size());
                AssertThat(buffer, EqualsContainer(expected));

                AssertThat(stream->bad(), Equals(0));
                AssertThat(stream->fail(), Equals(0));
                AssertThat(stream->eof(), Equals(0));
            };
            std::array<unsigned char, 2> p0{{'1', '2'}};
            std::array<unsigned char, 2> p1{{'3', '4'}};
            std::array<unsigned char, 2> p2{{'d', 'e'}};
            std::array<unsigned char, 3> p3{{'f', '0', '\n'}};

            test_read(p0);
            stream->seekg(0);
            test_read(p0);
            stream->seekg(2);
            test_read(p1);
            stream->seekg(-(p2.size() + p3.size()), std::ios_base::end);
            test_read(p2);
            stream->seekg(-2, std::ios_base::cur);
            test_read(p2);
            stream->seekg(0, std::ios_base::cur);
            test_read(p3);
            stream->seekg(2, std::ios_base::beg);
            test_read(p1);
        });

        it("file_device_subsource", [&]() {
            auto subsource = fdev.subsource(5,10);
            AssertThat(*subsource.size(), Equals(10));
            auto subsource2= subsource.subsource(5);
            AssertThat(*subsource2.size(), Equals(5));
            auto stream = subsource.stream();
            auto stream2 = subsource2.stream();
            std::string a,b,c;
            *stream2 >> b;
            AssertThat(b, Equals("9abc"));
            *stream >> a;
            AssertThat(a, Equals("5678"));
            *stream >> a;
            AssertThat(a, Equals("9abc"));
        });

    });
}
