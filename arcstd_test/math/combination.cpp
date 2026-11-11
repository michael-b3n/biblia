#include <arcstd/math/combination.hpp>
#include <arcstd/util/contains.hpp>

#include <catch2/catch_test_macros.hpp>

namespace arcstd::math
{

TEST_CASE("combination", "[math]")
{
  GIVEN("list of variants")
  {
    using matrix_type = std::vector<std::vector<int>>;
    const auto nested = matrix_type{
      {1, 2},
      {3, 4},
      {5, 6}
    };
    auto combinations = std::vector<std::vector<int>>{};
    for_each_combination(
      nested,
      [&](const auto& combination)
      {
        combinations.push_back(combination);
        return true;
      }
    );
    CHECK(combinations.size() == 8);
    CHECK(util::contains(combinations, std::vector{1, 3, 5}));
    CHECK(util::contains(combinations, std::vector{1, 3, 6}));
    CHECK(util::contains(combinations, std::vector{1, 4, 5}));
    CHECK(util::contains(combinations, std::vector{1, 4, 6}));
    CHECK(util::contains(combinations, std::vector{2, 3, 5}));
    CHECK(util::contains(combinations, std::vector{2, 3, 6}));
    CHECK(util::contains(combinations, std::vector{2, 4, 5}));
    CHECK(util::contains(combinations, std::vector{2, 4, 6}));
  }
}

} // namespace arcstd::math
