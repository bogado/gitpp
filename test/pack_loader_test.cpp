#include "pack_loader.hpp"
#include "sample-pack-data.hpp"

#include <iostream>
#include <fstream>

#include <bandit/bandit.h>

using namespace git;
using namespace bandit;

const static std::string SAMPLE_PACK_FILE(TEST_RESOURCE_PATH "/sample_pack.pack");
const static std::string SAMPLE_INDEX_FILE(TEST_RESOURCE_PATH "/sample_pack.idx");

void pack_data_test() {
    const static std::string PACK_FILE(SAMPLE_PACK_FILE);
    const static std::string INDEX_FILE(SAMPLE_INDEX_FILE);

    auto expected_begin = std::begin(data::get_expected_objects());
    auto expected_end   = std::end(data::get_expected_objects());

    describe("loading package files indexes test", [&]() {
        auto index_file_container = index_file_parser(INDEX_FILE);
        it("Item collection has expeced size", [&]{
            AssertThat(index_file_container.size(), Equals(std::distance(expected_begin, expected_end)));
        });
        it("Check Items collected:", [&]() {
            auto expected = expected_begin;
            int i = 0;
            std::stringstream item_desc;
            for (auto obj_loader : index_file_container) {
                item_desc.str("");
                item_desc << "Item " << i;
                i++;
                it(item_desc.str().c_str(), [&]() {
                    AssertThat(expected, Is().Not().EqualTo(expected_end));
                    AssertThat(obj_loader.get_name(), Equals(expected->name));
                    AssertThat(obj_loader.get_pack_offset(), Equals(expected->offset));

                });
                expected++;
            }
        });
    });
}
