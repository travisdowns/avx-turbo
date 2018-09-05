/*
 * unit-test.cpp
 */


#include "catch.hpp"

#include "../util.hpp"

#include <array>
#include <utility>
#include <cmath>

template <typename... Args>
std::vector<typename std::common_type<Args...>::type> v(Args&&... args) {
    return {args...};
}

TEST_CASE( "transform" ) {
    auto vec = std::vector<int>{1, 2, 3};
    REQUIRE(vec == v(1, 2, 3));

    REQUIRE(transformr(vec.begin(), vec.end(), [](int x){ return x + 10; }) == v(11, 12, 13));
    REQUIRE(transformv(vec,                    [](int x){ return x +  1; }) == v( 2,  3,  4));

    // check that everything compiles/works when the input and output types are different!
    std::vector<std::string> svec{"a", "aa", "aaa"};
    REQUIRE(transformv(svec, [](const std::string& s){ return s.size(); }) == v(1ul, 2ul,  3ul));
}

TEST_CASE( "remap" ) {
    REQUIRE(remap(0.2, 0, 1, 100, 200) == Approx(120));
}

std::pair<int,int> call_conc(const std::vector<std::pair<int,int>>& input) {
    return concurrency(input.begin(), input.end());
}

TEST_CASE( "concurrency" ) {

    REQUIRE(call_conc({ {1, 11}, {2, 4} }).first == 12);
    REQUIRE(call_conc({ {2, 4}, {1, 11} }).first == 12);

    REQUIRE(call_conc({ {2, 4}, {1, 11} }).second == 10);

    REQUIRE(call_conc({ {99, 100}, {1, 2} }).first  == 2);
    REQUIRE(call_conc({ {99, 100}, {1, 2} }).second == 2);

    REQUIRE(call_conc({ {-5, -4}, {100, 200}, {50, 60} }).first  == 111);
    REQUIRE(call_conc({ {-5, -4}, {100, 200}, {50, 60} }).second == 111);

    REQUIRE(call_conc({ {-5, -4}, {0, 100}, {50, 60} }).first  == 111);
    REQUIRE(call_conc({ {-5, -4}, {0, 100}, {50, 60} }).second == 101);

    REQUIRE(call_conc({ {1, 2}, {2, 3}, {3, 4} }).first  == 3);
    REQUIRE(call_conc({ {1, 2}, {2, 3}, {3, 4} }).second == 3);

    REQUIRE(call_conc({ {3, 4}, {1, 2}, {2, 3} }).first  == 3);
    REQUIRE(call_conc({ {3, 4}, {1, 2}, {2, 3} }).second == 3);

    REQUIRE(call_conc({ {1, 1}, {10, 10}, {10, 10}, {10, 10} }).first  == 0);
    REQUIRE(call_conc({ {1, 1}, {10, 10}, {10, 10}, {10, 10} }).second == 0);

}

double call_ratio(const std::vector<std::pair<int,int>>& input) {
    return conc_ratio(input.begin(), input.end());
}

TEST_CASE( "conc_ratio" ) {
    // 0 ranges
    REQUIRE(std::isnan(call_ratio({})));

    // 1 range
    REQUIRE(call_ratio({ {55,65} }) == Approx(1.0));

    // 2 ranges
    REQUIRE(call_ratio({ {55,65}, {55,65} })  == Approx(1.0));
    REQUIRE(call_ratio({ {55,65}, {65, 75} }) == Approx(0.0));

    // 3 ranges
    REQUIRE(call_ratio({ {0,10}, {0,3}, {0,7} }) == Approx(0.5));

    REQUIRE(call_ratio({ {0,10}, {0,3}, {0,7}, {11,11}, {11,11}, {11,11} }) == Approx(0.2));
}






