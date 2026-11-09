#include <arca/meta/pack.hpp>

#include <catch2/catch_test_macros.hpp>

#include <tuple>
#include <type_traits>
#include <variant>

namespace bibstd::meta
{

template<typename...>
struct test_pack final
{};

template<typename>
struct test_single final
{};

struct test_not_templated final
{};

TEST_CASE("pack is_packable", "[meta]")
{
  static_assert(is_pack_v<test_pack<>>);
  static_assert(is_pack_v<test_pack<int>>);
  static_assert(is_pack_v<test_pack<int, double>>);
  static_assert(is_pack_v<std::tuple<int, double>>);
  static_assert(is_pack_v<std::variant<int, double>>);
  static_assert(!is_pack_v<int>);
  static_assert(!is_pack_v<test_not_templated>);

  static_assert(packaged<std::tuple<int>>);
  static_assert(!packaged<int>);
}

TEST_CASE("pack unpack", "[meta]")
{
  static_assert(std::is_same_v<unpack_t<test_pack, std::tuple<int, double>>, test_pack<int, double>>);
  static_assert(std::is_same_v<unpack_t<std::tuple, test_pack<int, double>>, std::tuple<int, double>>);
  static_assert(std::is_same_v<unpack_t<std::variant, test_pack<int>>, std::variant<int>>);
  static_assert(std::is_same_v<unpack_t<test_pack, test_pack<>>, test_pack<>>);
}

TEST_CASE("pack add_to_pack", "[meta]")
{
  static_assert(std::is_same_v<add_to_pack_t<int, test_pack<>>, test_pack<int>>);
  static_assert(std::is_same_v<add_to_pack_t<int, test_pack<double>>, test_pack<int, double>>);
  static_assert(std::is_same_v<add_to_pack_t<int, std::tuple<double, char>>, std::tuple<int, double, char>>);
  // Adding a type already contained does not deduplicate.
  static_assert(std::is_same_v<add_to_pack_t<int, test_pack<int>>, test_pack<int, int>>);
}

TEST_CASE("pack remove_from_pack", "[meta]")
{
  static_assert(std::is_same_v<remove_from_pack_t<test_pack<int>>, test_pack<>>);
  static_assert(std::is_same_v<remove_from_pack_t<test_pack<int, double>>, test_pack<double>>);
  static_assert(std::is_same_v<remove_from_pack_t<std::tuple<int, double, char>>, std::tuple<double, char>>);
  // Removing is the inverse of adding from the left side.
  static_assert(std::is_same_v<remove_from_pack_t<add_to_pack_t<char, test_pack<int>>>, test_pack<int>>);
}

TEST_CASE("pack combine_pack", "[meta]")
{
  static_assert(std::is_same_v<combine_pack_t<test_pack<int>>, test_pack<int>>);
  static_assert(std::is_same_v<combine_pack_t<test_pack<int>, test_pack<double>>, test_pack<int, double>>);
  static_assert(std::is_same_v<combine_pack_t<test_pack<>, test_pack<double>>, test_pack<double>>);
  static_assert(std::is_same_v<combine_pack_t<test_pack<int>, test_pack<>>, test_pack<int>>);
  static_assert(
    std::is_same_v<combine_pack_t<test_pack<int>, test_pack<double>, test_pack<char>>, test_pack<int, double, char>>
  );
  // The resulting pack type is the one of the first pack.
  static_assert(std::is_same_v<combine_pack_t<std::tuple<int>, test_pack<double>>, std::tuple<int, double>>);
}

TEST_CASE("pack split_pack", "[meta]")
{
  using pack_type = test_pack<int, double, char, float>;

  static_assert(std::is_same_v<split_pack<pack_type, 0>::first_type, test_pack<>>);
  static_assert(std::is_same_v<split_pack<pack_type, 0>::second_type, pack_type>);
  static_assert(std::is_same_v<split_pack<pack_type, 4>::first_type, pack_type>);
  static_assert(std::is_same_v<split_pack<pack_type, 4>::second_type, test_pack<>>);
  static_assert(std::is_same_v<split_pack<pack_type, 1>::first_type, test_pack<int>>);
  static_assert(std::is_same_v<split_pack<pack_type, 1>::second_type, test_pack<double, char, float>>);
  static_assert(std::is_same_v<split_pack<pack_type, 2>::first_type, test_pack<int, double>>);
  static_assert(std::is_same_v<split_pack<pack_type, 2>::second_type, test_pack<char, float>>);
  static_assert(std::is_same_v<split_pack<pack_type, 3>::first_type, test_pack<int, double, char>>);
  static_assert(std::is_same_v<split_pack<pack_type, 3>::second_type, test_pack<float>>);
}

TEST_CASE("pack pack_n_types", "[meta]")
{
  static_assert(std::is_same_v<pack_n_types_t<test_pack, int, 0>, test_pack<>>);
  static_assert(std::is_same_v<pack_n_types_t<test_pack, int, 1>, test_pack<int>>);
  static_assert(std::is_same_v<pack_n_types_t<test_pack, int, 3>, test_pack<int, int, int>>);
  static_assert(std::is_same_v<pack_n_types_t<std::tuple, double, 2>, std::tuple<double, double>>);
}

TEST_CASE("pack type_index", "[meta]")
{
  using pack_type = test_pack<int, double, char>;

  static_assert(type_index_v<pack_type, int> == 0);
  static_assert(type_index_v<pack_type, double> == 1);
  static_assert(type_index_v<pack_type, char> == 2);
  // A type not contained yields the pack size.
  static_assert(type_index_v<pack_type, float> == 3);
  static_assert(type_index_v<test_pack<>, int> == 0);
  // The first occurrence is reported.
  static_assert(type_index_v<test_pack<int, double, int>, int> == 0);
}

TEST_CASE("pack pack_info", "[meta]")
{
  static_assert(pack_info<test_pack<int>>::size == 1);
  static_assert(std::is_same_v<pack_info<test_pack<int>>::first_type, int>);
  static_assert(std::is_same_v<pack_info<test_pack<int>>::last_type, int>);
  static_assert(std::is_same_v<pack_info<test_pack<int>>::type_at<0>, int>);

  using pack_type = test_pack<int, double, char>;
  static_assert(pack_info<pack_type>::size == 3);
  static_assert(std::is_same_v<pack_info<pack_type>::first_type, int>);
  static_assert(std::is_same_v<pack_info<pack_type>::last_type, char>);
  static_assert(std::is_same_v<pack_info<pack_type>::type_at<0>, int>);
  static_assert(std::is_same_v<pack_info<pack_type>::type_at<1>, double>);
  static_assert(std::is_same_v<pack_info<pack_type>::type_at<2>, char>);

  static_assert(pack_info<std::tuple<int, double>>::size == 2);
  static_assert(std::is_same_v<pack_info<test_single<int>>::first_type, int>);
}

} // namespace bibstd::meta
