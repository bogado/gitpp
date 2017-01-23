#include "pack_index.hpp"

#include "object_tests.hpp"

#include <iostream>
#include <fstream>

#include <bandit/bandit.h>

#include "sample-pack-data.hpp"

using namespace git;
using namespace bandit;

const static std::string SAMPLE_INDEX_FILE(TEST_RESOURCE_PATH "/sample_pack.idx");

void pack_index_test() {
    const static std::string INDEX_FILE(SAMPLE_INDEX_FILE);

    describe("loading package files indexes test", [&]() {
        auto index_file_container = index_file_parser(INDEX_FILE);
        test_collected("index loader", index_file_container, data::get_expected_objects(),
            [&](auto& obtained, auto& expected) {
                AssertThat(obtained.get_name(), Equals(expected->name))
                AssertThat(obtained.get_pack_offset(), Equals(expected->offset));
            }
        );
    });
}

