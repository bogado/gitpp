#ifndef PACK_LOADER_TEST_HPP_INCLUDED
#define PACK_LOADER_TEST_HPP_INCLUDED

#ifndef TESTS_READY
#error This files has to be included by test.cpp alone.
#endif

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

    auto expected = std::begin(data::get_expected_objects());
    auto expected_end = std::end(data::get_expected_objects());

    describe("loading package files indexes test", [&]() {
        auto index_file_container = index_file_parser(INDEX_FILE);
        it("Item collection has expeced size", [&]{
            AssertThat(index_file_container.size(), Equals(std::distance(expected, expected_end)));
        });
        it("Item collecting content matches expected content", [&]() {
            for (auto obj_loader : index_file_container) {
                AssertThat(obj_loader.name(), Equals(expected->name));
                AssertThat(obj_loader.type(), Equals(expected->type));
                AssertThat(obj_loader.size(), Equals(expected->size));
                AssertThat(obj_loader.pack_size(), Equals(expected->pack_size));
                expected++;
                AssertThat(expected, Is().Not().EqualTo(expected_end));
            }
        });
    });
}

#endif
