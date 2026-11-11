#include <arcstd/math/arithmetic.hpp>
#include <arcstd/math/is_equal.hpp>
#include <arcstd/meta/pack.hpp>

#include <catch2/catch_test_macros.hpp>
#include <limits>

namespace bibstd::math
{

template<meta::packaged P>
struct for_each_type;

template<template<typename...> typename P, typename... Args>
struct for_each_type<P<Args...>>
{
  static constexpr auto all_of(auto&& f) -> bool { return (f.template operator()<Args>() && ...); };
};

// Constants
template<typename T>
constexpr auto max = std::numeric_limits<T>::max();
template<typename T>
constexpr auto min = std::numeric_limits<T>::lowest();

TEST_CASE("Integer arithmetic", "[math]")
{
  // Typedefs
  using unsigned_types = std::tuple<std::uint8_t, std::uint16_t, std::uint32_t, std::uint64_t>;
  using signed_types = std::tuple<std::int8_t, std::int16_t, std::int32_t, std::int64_t>;
  using all_types = meta::combine_pack_t<unsigned_types, signed_types>;

  using all_t = for_each_type<all_types>;
  using signed_t = for_each_type<signed_types>;
  using unsigned_t = for_each_type<unsigned_types>;

  // clang-format off
  // Addition
  static_assert(all_t::all_of([]<typename T>() { return is_equal(*arithmetic::add(T{1}, T{1}), T{2}); }));
  static_assert(all_t::all_of([]<typename T>() { return is_equal(*arithmetic::add(T{1}, min<T>), static_cast<T>(min<T> + T{1})); }));
  static_assert(all_t::all_of([]<typename T>() { return arithmetic::add(T{1}, max<T>).error() == arithmetic::error_code::overflow; }));
  static_assert(all_t::all_of([]<typename T>() { return arithmetic::add(max<T>, max<T>).error() == arithmetic::error_code::overflow; }));
  static_assert(signed_t::all_of([]<typename T>() { return is_equal(*arithmetic::add(T{-1}, T{-1}), T{-2}); }));
  static_assert(signed_t::all_of([]<typename T>() { return is_equal(*arithmetic::add(T{-1}, max<T>), static_cast<T>(max<T> - T{1})); }));
  static_assert(signed_t::all_of([]<typename T>() { return arithmetic::add(T{-1}, min<T>).error() == arithmetic::error_code::underflow; }));
  static_assert(signed_t::all_of([]<typename T>() { return arithmetic::add(min<T>, min<T>).error() == arithmetic::error_code::underflow; }));
  static_assert(signed_t::all_of([]<typename T>() { return is_equal(*arithmetic::add(max<T>, min<T>), T{-1}); }));
  static_assert(unsigned_t::all_of([]<typename T>() { return is_equal(*arithmetic::add(min<T>, max<T>), max<T>); }));

  // Subtraction
  static_assert(all_t::all_of([]<typename T>() { return is_equal(*arithmetic::subtract(T{2}, T{1}), T{1}); }));
  static_assert(all_t::all_of([]<typename T>() { return is_equal(*arithmetic::subtract(max<T>, T{1}), static_cast<T>(max<T> - T{1})); }));
  static_assert(all_t::all_of([]<typename T>() { return arithmetic::subtract(min<T>, T{1}).error() == arithmetic::error_code::underflow; }));
  static_assert(signed_t::all_of([]<typename T>() { return arithmetic::subtract(T{1}, min<T>).error() == arithmetic::error_code::overflow; }));
  static_assert(signed_t::all_of([]<typename T>() { return is_equal(*arithmetic::subtract(T{1}, T{2}), T{-1}); }));
  static_assert(signed_t::all_of([]<typename T>() { return is_equal(*arithmetic::subtract(T{1}, max<T>), static_cast<T>(min<T> + T{2})); }));
  static_assert(signed_t::all_of([]<typename T>() { return is_equal(*arithmetic::subtract(min<T>, T{-1}), static_cast<T>(min<T> + T{1})); }));
  static_assert(signed_t::all_of([]<typename T>() { return is_equal(*arithmetic::subtract(T{-1}, min<T>), max<T>); }));
  static_assert(signed_t::all_of([]<typename T>() { return is_equal(*arithmetic::subtract(T{-1}, max<T>), min<T>); }));
  static_assert(signed_t::all_of([]<typename T>() { return arithmetic::subtract(max<T>, T{-1}).error() == arithmetic::error_code::overflow; }));
  static_assert(signed_t::all_of([]<typename T>() { return arithmetic::subtract(min<T>, T{1}).error() == arithmetic::error_code::underflow; }));
  static_assert(signed_t::all_of([]<typename T>() { return arithmetic::subtract(T{-1}, max<T>) == min<T>; }));

  // Multiplication
  static_assert(all_t::all_of([]<typename T>() { return is_equal(*arithmetic::multiply(T{2}, T{2}), T{4}); }));
  static_assert(all_t::all_of([]<typename T>() { return is_equal(*arithmetic::multiply(T{1}, max<T>), max<T>); }));
  static_assert(all_t::all_of([]<typename T>() { return is_equal(*arithmetic::multiply(T{1}, min<T>), min<T>); }));
  static_assert(all_t::all_of([]<typename T>() { return arithmetic::multiply(T{2}, max<T>).error() == arithmetic::error_code::overflow; }));
  static_assert(signed_t::all_of([]<typename T>() { return is_equal(*arithmetic::multiply(T{-2}, T{2}), T{-4}); }));
  static_assert(signed_t::all_of([]<typename T>() { return is_equal(*arithmetic::multiply(T{2}, T{-2}), T{-4}); }));
  static_assert(signed_t::all_of([]<typename T>() { return is_equal(*arithmetic::multiply(T{-2}, T{-2}), T{4}); }));
  static_assert(signed_t::all_of([]<typename T>() { return is_equal(*arithmetic::multiply(T{-1}, max<T>), static_cast<T>(min<T> + T{1})); }));
  static_assert(signed_t::all_of([]<typename T>() { return arithmetic::multiply(T{-1}, min<T>).error() == arithmetic::error_code::overflow; }));
  static_assert(signed_t::all_of([]<typename T>() { return arithmetic::multiply(T{-2}, max<T>).error() == arithmetic::error_code::underflow; }));
  static_assert(signed_t::all_of([]<typename T>() { return arithmetic::multiply(T{2}, min<T>).error() == arithmetic::error_code::underflow; }));

  // Division
  static_assert(all_t::all_of([]<typename T>() { return is_equal(*arithmetic::divide(T{3}, T{2}), T{1}); }));
  static_assert(all_t::all_of([]<typename T>() { return is_equal(*arithmetic::divide(T{2}, T{3}), T{0}); }));
  static_assert(all_t::all_of([]<typename T>() { return arithmetic::divide(T{1}, T{0}).error() == arithmetic::error_code::overflow; }));
  static_assert(signed_t::all_of([]<typename T>() { return is_equal(*arithmetic::divide(T{3}, T{-2}), T{-1}); }));
  // clang-format on
}

TEST_CASE("Floating point arithmetic", "[math]")
{
  // Typedefs
  using fp_types = std::tuple<float, double>;
  using fp_t = for_each_type<fp_types>;

  // clang-format off
  // Addition
  static_assert(fp_t::all_of([]<typename T>() { return is_equal(*arithmetic::add(T{1}, T{1}), T{2}); }));
  static_assert(fp_t::all_of([]<typename T>() { return is_equal(*arithmetic::add(T{1}, T{-1}), T{0}); }));
  static_assert(fp_t::all_of([]<typename T>() { return is_equal(*arithmetic::add(T{-1}, T{-1}), T{-2}); }));
  static_assert(fp_t::all_of([]<typename T>() { return is_equal(*arithmetic::add(max<T>, T{1}), max<T>); }));
  CHECK(fp_t::all_of([]<typename T>() { return arithmetic::add(max<T>, max<T>).error() == arithmetic::error_code::overflow; }));
  CHECK(fp_t::all_of([]<typename T>() { return arithmetic::add(min<T>, min<T>).error() == arithmetic::error_code::underflow; }));

  // Subtraction
  static_assert(fp_t::all_of([]<typename T>() { return is_equal(*arithmetic::subtract(T{2}, T{1}), T{1}); }));
  static_assert(fp_t::all_of([]<typename T>() { return is_equal(*arithmetic::subtract(T{1}, T{2}), T{-1}); }));
  static_assert(fp_t::all_of([]<typename T>() { return is_equal(*arithmetic::subtract(T{1}, T{1}), T{0}); }));
  static_assert(fp_t::all_of([]<typename T>() { return is_equal(*arithmetic::subtract(min<T>, T{1}), min<T>); }));
  static_assert(fp_t::all_of([]<typename T>() { return is_equal(*arithmetic::subtract(min<T>, min<T>), T{0}); }));
  CHECK(fp_t::all_of([]<typename T>() { return arithmetic::subtract(max<T>, min<T>).error() == arithmetic::error_code::overflow; }));
  CHECK(fp_t::all_of([]<typename T>() { return arithmetic::subtract(min<T>, max<T>).error() == arithmetic::error_code::underflow; }));

  // Multiplication
  static_assert(fp_t::all_of([]<typename T>() { return is_equal(*arithmetic::multiply(T{2}, T{2}), T{4}); }));
  static_assert(fp_t::all_of([]<typename T>() { return is_equal(*arithmetic::multiply(T{1}, min<T>), min<T>); }));
  static_assert(fp_t::all_of([]<typename T>() { return is_equal(*arithmetic::multiply(T{1}, max<T>), max<T>); }));
  static_assert(fp_t::all_of([]<typename T>() { return is_equal(*arithmetic::multiply(T{0}, min<T>), T{0}); }));
  static_assert(fp_t::all_of([]<typename T>() { return is_equal(*arithmetic::multiply(T{0}, max<T>), T{0}); }));
  CHECK(fp_t::all_of([]<typename T>() { return arithmetic::multiply(max<T>, T{2}).error() == arithmetic::error_code::overflow; }));
  CHECK(fp_t::all_of([]<typename T>() { return arithmetic::multiply(min<T>, T{2}).error() == arithmetic::error_code::underflow; }));

  // Division
  static_assert(fp_t::all_of([]<typename T>() { return is_equal(*arithmetic::divide(T{3}, T{2}), T{1.5}); }));
  static_assert(fp_t::all_of([]<typename T>() { return is_equal(*arithmetic::divide(T{1}, T{2}), T{0.5}); }));
  CHECK(fp_t::all_of([]<typename T>() { return arithmetic::divide(T{0}, T{0}).error() == arithmetic::error_code::undefined; }));
  CHECK(fp_t::all_of([]<typename T>() { return arithmetic::divide(T{1}, T{0}).error() == arithmetic::error_code::overflow; }));
  CHECK(fp_t::all_of([]<typename T>() { return arithmetic::divide(T{-1}, T{0}).error() == arithmetic::error_code::underflow; }));
  // clang-format on
}

} // namespace bibstd::math
