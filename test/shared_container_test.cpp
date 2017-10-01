#include "util/shared_container.hpp"

#include <vector>

#include <bandit/bandit.h>

using namespace bandit;
using namespace git;

using shared_vector = shared_container<std::vector<int>>;

shared_vector create_shared_vector(int elements) {
    shared_vector result;
    for (int i = 0; i < elements; i++) {
        result->push_back(i+1);
    }
    return result;
}

void shared_container_test() {
    describe("shared container", [&]() {

        it("container comparison", [&]() {
            auto v1 = make_shared_container<std::vector<int>>({1,2,3,4});
            auto v2 = create_shared_vector(4);
            AssertThat(v1 == v2, Equals(true));
            v2->push_back(5);
            AssertThat(v1 == v2, Equals(false));
        });

        it("container size", [&]() {
            auto v1 = shared_container<int[4]>();
            (*v1)[0] = 1; (*v1)[1] = 2; (*v1)[2] = 3; (*v1)[3] = 4;

            int local[4] = {1, 2, 3, 4};
            auto v2 = shared_container(local);

            AssertThat(v1.size(), Equals(4));
            AssertThat(v2.size(), Equals(4));
        });
    });
}
