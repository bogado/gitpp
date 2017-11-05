#include "pack/index.hpp"

#include "object_tests.hpp"

#include <iostream>
#include <fstream>

#include <bandit/bandit.h>

#include "sample-pack-data.hpp"

using namespace git;
using namespace bandit;
using namespace snowhouse;

const static std::string SAMPLE_INDEX_FILE(TEST_RESOURCE_PATH "/sample_pack.idx");

void pack_index_test() {
    const static std::string INDEX_FILE(SAMPLE_INDEX_FILE);

    describe("loading package files indexes test", [&]() {
        auto index_file_container = index_file_parser(INDEX_FILE);
        auto expected_items = data::get_expected_objects();

        auto verify_object = [&](const auto& obtained, const auto expected) {
            AssertThat(obtained.get_name(), Equals(expected->name));
            AssertThat(obtained.get_pack_offset(), Equals(expected->offset));
        };

        static constexpr auto SUB_LEN = 6;

        for (auto expected_object: expected_items) {
            it(("index by name " + expected_object.name).c_str(), [&]() {
                verify_object(index_file_container[expected_object.name], &expected_object);
            });
            auto short_name = expected_object.name.substr(0, SUB_LEN);
            it(("index by short name " + short_name).c_str(), [&]() {
                verify_object(index_file_container[short_name], &expected_object);
            });
        }

        test_collected("index loader", index_file_container, data::get_expected_objects(), verify_object);
    });
}

