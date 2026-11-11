#include <arcstd/util/const_map.hpp>
#include <arcstd/util/exception.hpp>

#include <catch2/catch_test_macros.hpp>

#include <iterator>
#include <string_view>
#include <type_traits>
#include <utility>

namespace arcstd::util
{

enum class const_map_test_enum
{
  a,
  b,
  c
};

// Constants
constexpr auto test_bimap = make_const_bimap<const_map_test_enum, std::string_view>({
  {const_map_test_enum::a, "a"},
  {const_map_test_enum::b, "b"},
  {const_map_test_enum::c, "c"},
});
constexpr auto test_map = make_const_map<int, int>({
  {0, 1},
  {1, 1},
  {2, 2},
});

TEST_CASE("const_map typedefs and size", "[util]")
{
  static_assert(std::is_same_v<decltype(test_bimap)::first_type, const_map_test_enum>);
  static_assert(std::is_same_v<decltype(test_bimap)::second_type, std::string_view>);
  static_assert(test_bimap.size() == 3);
  static_assert(test_map.size() == 3);
}

TEST_CASE("const_map contains", "[util]")
{
  static_assert(test_bimap.contains(const_map_test_enum::a));
  static_assert(test_bimap.contains(const_map_test_enum::c));
  static_assert(test_bimap.contains(std::string_view{"a"}));
  static_assert(test_bimap.contains(std::string_view{"c"}));
  static_assert(!test_bimap.contains(std::string_view{"d"}));

  // A unidirectional map only looks at the first elements.
  static_assert(test_map.contains(0));
  static_assert(test_map.contains(2));
  static_assert(!test_map.contains(3));
}

TEST_CASE("const_map at", "[util]")
{
  static_assert(test_bimap.at(const_map_test_enum::a) == "a");
  static_assert(test_bimap.at(const_map_test_enum::b) == "b");
  static_assert(test_bimap.at(std::string_view{"c"}) == const_map_test_enum::c);

  static_assert(test_map.at(0) == 1);
  static_assert(test_map.at(1) == 1);
  static_assert(test_map.at(2) == 2);
}

TEST_CASE("const_map at throws for unknown keys", "[util]")
{
  CHECK_THROWS_AS(test_bimap.at(static_cast<const_map_test_enum>(42)), util::exception);
  CHECK_THROWS_AS(test_bimap.at(std::string_view{"d"}), util::exception);
  CHECK_THROWS_AS(test_map.at(3), util::exception);
}

TEST_CASE("const_map iteration", "[util]")
{
  static_assert(std::ranges::distance(test_bimap) == 3);
  static_assert(test_bimap.begin()->first == const_map_test_enum::a);
  static_assert(test_bimap.cbegin()->second == "a");
  static_assert(std::prev(test_bimap.end())->first == const_map_test_enum::c);
  static_assert(test_bimap.rbegin()->first == const_map_test_enum::c);
  static_assert(test_bimap.crbegin()->second == "c");
  static_assert(test_bimap.begin() == test_bimap.cbegin());
  static_assert(test_bimap.end() == test_bimap.cend());
  static_assert(test_bimap.rend() == test_bimap.crend());

  auto count = 0;
  for(const auto& [key, value] : test_bimap)
  {
    CHECK(test_bimap.at(key) == value);
    ++count;
  }
  CHECK(count == 3);
}

TEST_CASE("const_map type traits", "[util]")
{
  static_assert(is_const_map_v<const_map<int, double, 1>>);
  static_assert(!is_const_map_v<int>);
  static_assert(const_map_type<const_map<int, double, 1>>);
  static_assert(!const_map_type<int>);
}

TEST_CASE("const_map rejects duplicated keys", "[util]")
{
  using map_type = const_map<int, int, 2, false>;
  CHECK_THROWS_AS(map_type(std::pair{0, 1}, std::pair{0, 2}), util::exception);

  using bimap_type = const_map<int, std::string_view, 2, true>;
  CHECK_THROWS_AS(
    bimap_type(std::pair<int, std::string_view>{0, "a"}, std::pair<int, std::string_view>{0, "b"}), util::exception
  );
  // A bidirectional map also rejects duplicated values.
  CHECK_THROWS_AS(
    bimap_type(std::pair<int, std::string_view>{0, "a"}, std::pair<int, std::string_view>{1, "a"}), util::exception
  );
  // The same value is fine in a unidirectional map.
  CHECK_NOTHROW(const_map<int, int, 2, false>(std::pair{0, 1}, std::pair{1, 1}));
}

} // namespace arcstd::util
