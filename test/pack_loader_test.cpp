#include "pack_loader.hpp"

#include "object_tests.hpp"

#include <iostream>
#include <fstream>

#include <bandit/bandit.h>

#include "sample-pack-data.hpp"

using namespace git;
using namespace bandit;

const static std::string SAMPLE_PACK_FILE_BASE(TEST_RESOURCE_PATH "/sample_pack");

void pack_data_test() {
    describe("loading pack files test", [&]() {
        auto pack_file_container = pack_file_parser(SAMPLE_PACK_FILE_BASE);
        test_collected("pack loader", pack_file_container, data::get_expected_objects(),
            [&](auto& obtained, auto& expected) {
                AssertThat(obtained.get_name(), Equals(expected->name));
                AssertThat(obtained.get_pack_offset(), Equals(expected->offset));
                AssertThat(obtained.get_type(), Equals(expected->type));
                AssertThat(obtained.get_size(), Equals(expected->size));
                AssertThat(obtained.get_pack_size(), Equals(expected->pack_size));
                AssertThat(obtained.get_pack_depth(), Equals(expected->depth));
                if (obtained.get_delta_parent()) {
                    AssertThat(obtained.get_delta_parent().get_name(), Equals(expected->parent));
                } else {
                    AssertThat("", Equals(expected->parent));
                }
            }
        );
    });
}
