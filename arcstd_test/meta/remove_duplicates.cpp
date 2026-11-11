#include <arcstd/meta/remove_duplicates.hpp>

#include <catch2/catch_test_macros.hpp>

#include <tuple>
#include <type_traits>

namespace arcstd::meta
{

template<typename...>
struct remove_duplicates_test_pack final
{};

TEST_CASE("remove_duplicates", "[meta]")
{
  using pack = remove_duplicates_test_pack<int, double, char>;

  static_assert(std::is_same_v<remove_duplicates_t<remove_duplicates_test_pack<int>>, remove_duplicates_test_pack<int>>);
  static_assert(std::is_same_v<remove_duplicates_t<pack>, pack>);
  static_assert(std::is_same_v<remove_duplicates_t<remove_duplicates_test_pack<int, int>>, remove_duplicates_test_pack<int>>);
  static_assert(
    std::is_same_v<remove_duplicates_t<remove_duplicates_test_pack<int, int, int>>, remove_duplicates_test_pack<int>>
  );
  static_assert(
    std::is_same_v<remove_duplicates_t<remove_duplicates_test_pack<int, double, int>>, remove_duplicates_test_pack<double, int>>
  );
  static_assert(std::is_same_v<
                remove_duplicates_t<remove_duplicates_test_pack<int, double, int, double>>,
                remove_duplicates_test_pack<int, double>>);
  static_assert(std::is_same_v<remove_duplicates_t<std::tuple<int, int, double>>, std::tuple<int, double>>);

  // Cv-qualified types count as distinct types.
  static_assert(std::is_same_v<
                remove_duplicates_t<remove_duplicates_test_pack<int, const int>>,
                remove_duplicates_test_pack<int, const int>>);
}

} // namespace arcstd::meta
