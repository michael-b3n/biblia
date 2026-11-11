#include <arcstd/util/const_variant_map.hpp>
#include <arcstd/util/exception.hpp>

#include <catch2/catch_test_macros.hpp>

#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace arcstd::util
{
namespace
{

using int_to_double = std::pair<int, double>;
using string_to_int = std::pair<std::string_view, int>;
using char_to_double = std::pair<char, double>;

///
/// Create a map with three different key types and two different value types.
///
constexpr auto make_test_map() -> auto
{
  return const_variant_map{
    int_to_double{    1, 1.5},
    string_to_int{"key",   7},
    char_to_double{  'a', 2.5}
  };
}

} // namespace

TEST_CASE("const_variant_map typedefs", "[util]")
{
  using map_type = decltype(make_test_map());

  static_assert(map_type::size == 3);
  static_assert(std::is_same_v<map_type::key_variant_type, std::variant<int, std::string_view, char>>);
  // Duplicated value types are collapsed into a single variant alternative.
  static_assert(std::is_same_v<map_type::value_variant_type, std::variant<int, double>>);
}

TEST_CASE("const_variant_map contains", "[util]")
{
  const auto map = make_test_map();

  CHECK(map.contains(1));
  CHECK(map.contains(std::string_view{"key"}));
  CHECK(map.contains('a'));
  CHECK(!map.contains(2));
  CHECK(!map.contains(std::string_view{"other"}));
  CHECK(!map.contains('b'));
}

TEST_CASE("const_variant_map at", "[util]")
{
  const auto map = make_test_map();

  CHECK(std::get<double>(map.at(1)) == 1.5);
  CHECK(std::get<int>(map.at(std::string_view{"key"})) == 7);
  CHECK(std::get<double>(map.at('a')) == 2.5);

  CHECK_THROWS_AS(map.at(2), util::exception);
  CHECK_THROWS_AS(map.at('b'), util::exception);
}

TEST_CASE("const_variant_map visit", "[util]")
{
  const auto map = make_test_map();

  auto visited = 0.0;
  CHECK(map.visit(1, [&](const auto& value) { visited = static_cast<double>(value); }));
  CHECK(visited == 1.5);

  CHECK(map.visit(std::string_view{"key"}, [&](const auto& value) { visited = static_cast<double>(value); }));
  CHECK(visited == 7.0);

  // An unknown key leaves the callable uncalled.
  visited = 0.0;
  CHECK(!map.visit(42, [&](const auto& value) { visited = static_cast<double>(value); }));
  CHECK(visited == 0.0);
}

TEST_CASE("const_variant_map visit_until", "[util]")
{
  const auto map = make_test_map();

  auto count = 0;
  map.visit_until(
    [&](const auto&, const auto&)
    {
      ++count;
      return true;
    }
  );
  CHECK(count == 3);

  // Returning false breaks out of the iteration.
  count = 0;
  map.visit_until(
    [&](const auto&, const auto&)
    {
      ++count;
      return false;
    }
  );
  CHECK(count == 1);

  // Keys and values arrive as variants of the map key and value types.
  auto keys = std::vector<std::size_t>{};
  map.visit_until(
    [&](const auto& key, const auto&)
    {
      keys.push_back(key.index());
      return true;
    }
  );
  CHECK(keys == std::vector<std::size_t>{0, 1, 2});
}

TEST_CASE("const_variant_map rejects duplicated keys", "[util]")
{
  CHECK_THROWS_AS(const_variant_map(int_to_double{1, 1.5}, int_to_double{1, 2.5}), util::exception);
  // Equal values under different keys are fine.
  CHECK_NOTHROW(const_variant_map(int_to_double{1, 1.5}, int_to_double{2, 1.5}));
}

} // namespace arcstd::util
