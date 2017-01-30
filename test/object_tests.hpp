#ifndef OBJECT_TESTS_HPP_INCLUDED
#define OBJECT_TESTS_HPP_INCLUDED

#include <string>

#include <bandit/bandit.h>

using namespace bandit;

template <typename COLLECTED, typename CHECK, typename EXPECTED>
void test_collected(std::string description, COLLECTED& items, const EXPECTED& expected_objects, CHECK check) {
    auto expected_begin = std::begin(expected_objects);
    auto expected_end   = std::end(expected_objects);
    auto expected = expected_begin;

    std::stringstream it_desc;

    it_desc << description << ": expected size";
    it(it_desc.str().c_str(), [&]{
        AssertThat(items.size(), Equals(std::distance(expected_begin, expected_end)));
    });

    int i = 0;
    for(auto item : items) {
        it_desc.str("");
        it_desc << description << ": item #" << i++;
        AssertThat(expected, Is().Not().EqualTo(expected_end));

        it(it_desc.str().c_str(), [&]() {
            check(item, expected);
        });
        expected++;
    }
}

#endif
