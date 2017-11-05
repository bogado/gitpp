#ifndef OBJECT_TESTS_HPP_INCLUDED
#define OBJECT_TESTS_HPP_INCLUDED

#include <string>

#include <bandit/bandit.h>

template <typename COLLECTED, typename CHECK, typename EXPECTED>
void test_collected(std::string description, COLLECTED& items, const EXPECTED& expected_objects, CHECK check) {
    using namespace snowhouse;
    auto expected_begin = std::begin(expected_objects);
    auto expected_end   = std::end(expected_objects);
    auto expected = expected_begin;

    std::stringstream it_desc;

    it_desc << description << ": expected size";
    bandit::it(it_desc.str().c_str(), [&]{
        AssertThat(items.size(), Equals(std::distance(expected_begin, expected_end)));
    });

    int i = 0;
    for(const auto& item : items) {
        it_desc.str("");
        it_desc << description << ": item #" << i++;

        bandit::it(it_desc.str().c_str(), [&]() {
            AssertThat(expected, Is().Not().EqualTo(expected_end));
            check(item, expected);
        });
        expected++;
    }
}

#endif
