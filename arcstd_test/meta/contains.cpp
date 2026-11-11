#include <arcstd/meta/contains.hpp>

#include <catch2/catch_test_macros.hpp>

#include <tuple>
#include <variant>

namespace bibstd::meta
{

template<typename...>
struct contains_test_pack final
{};

TEST_CASE("contains", "[meta]")
{
  using pack_type = contains_test_pack<int, double, char>;

  static_assert(contains_v<pack_type, int>);
  static_assert(contains_v<pack_type, double>);
  static_assert(contains_v<pack_type, char>);
  static_assert(!contains_v<pack_type, float>);
  static_assert(!contains_v<pack_type, void>);

  static_assert(!contains_v<pack_type, const int>);
  static_assert(!contains_v<pack_type, int&>);
  static_assert(contains_v<contains_test_pack<const int>, const int>);

  static_assert(contains_v<std::tuple<int, double>, double>);
  static_assert(!contains_v<std::tuple<int, double>, char>);
  static_assert(contains_v<std::variant<int, double>, int>);

  static_assert(contains_v<contains_test_pack<int>, int>);
  static_assert(!contains_v<contains_test_pack<int>, double>);
}

} // namespace bibstd::meta
