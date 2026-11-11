#include <arcstd/meta/for_each.hpp>

#include <catch2/catch_test_macros.hpp>

#include <tuple>
#include <type_traits>
#include <variant>
#include <vector>

namespace arcstd::meta
{

template<typename...>
struct for_each_test_pack final
{};

template<typename T>
struct wrapper final
{};

TEST_CASE("for_each on non pack type", "[meta]")
{
  static_assert(std::is_same_v<for_each_t<int, wrapper>, wrapper<int>>);
  static_assert(std::is_same_v<for_each_t<double, std::vector>, std::vector<double>>);
}

TEST_CASE("for_each on pack type", "[meta]")
{
  static_assert(std::is_same_v<for_each_t<for_each_test_pack<int>, wrapper>, for_each_test_pack<wrapper<int>>>);
  static_assert(
    std::is_same_v<for_each_t<for_each_test_pack<int, double>, wrapper>, for_each_test_pack<wrapper<int>, wrapper<double>>>
  );
  static_assert(std::is_same_v<
                for_each_t<for_each_test_pack<int, double, char>, wrapper>,
                for_each_test_pack<wrapper<int>, wrapper<double>, wrapper<char>>>);
  static_assert(std::is_same_v<for_each_t<std::tuple<int, double>, wrapper>, std::tuple<wrapper<int>, wrapper<double>>>);
  static_assert(std::is_same_v<for_each_t<std::variant<int>, wrapper>, std::variant<wrapper<int>>>);

  static_assert(
    std::is_same_v<for_each_t<for_each_t<for_each_test_pack<int>, wrapper>, wrapper>, for_each_test_pack<wrapper<wrapper<int>>>>
  );
}

TEST_CASE("for_each with standard templates", "[meta]")
{
  static_assert(std::is_same_v<
                for_each_t<for_each_test_pack<int, double>, std::vector>,
                for_each_test_pack<std::vector<int>, std::vector<double>>>);
}

} // namespace arcstd::meta
