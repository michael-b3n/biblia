#pragma once

#include "arcstd/meta/pack.hpp"

#include <tuple>
#include <type_traits>

namespace bibstd::meta
{
namespace detail
{

///
/// Helper type to allow any type to be constructed.
///
struct any_type final
{
  any_type() = default;
  template<typename T>
  operator T&&()
  {
    return std::declval<T&&>();
  }
  template<typename T>
  operator T&()
  {
    return std::declval<T&>();
  }
};

///
/// Constructable concept and helpers.
///
template<typename T, typename... Args>
concept constructable_with_helper = requires { std::decay_t<T>{Args{}...}; };
template<typename T, packaged P>
struct is_constructable_with : std::false_type
{};
template<typename T, template<typename...> typename P, typename... Args>
  requires constructable_with_helper<T, Args...>
struct is_constructable_with<T, P<Args...>> : std::true_type
{};
template<typename T, std::size_t N>
concept constructable_with = is_constructable_with<T, pack_n_types_t<std::tuple, any_type, N>>::value;

template<std::size_t N>
struct size_t_constant final
{
  static constexpr std::size_t value = N;
};

///
/// Constructing arguments count helper.
///
template<typename T, std::size_t... Is>
struct constructing_params_count_helper final
{};
template<typename T>
struct constructing_params_count_helper<T> final
{
  using type = size_t_constant<0>;
};
template<typename T, std::size_t I, std::size_t... Is>
struct constructing_params_count_helper<T, I, Is...> final
{
  using type =
    std::conditional_t<constructable_with<T, I>, size_t_constant<I>, typename constructing_params_count_helper<T, Is...>::type>;
};

///
/// Converts a variable number of arguments into a tuple.
/// \return a tuple containing the forwarded arguments
///
[[nodiscard]] constexpr auto to_tuple(auto&&... args) -> auto
{
  return std::make_tuple(std::forward<decltype(args)>(args)...);
}

///
/// Unfolder functions for different numbers of elements
/// These functions are used to unfold objects into tuples.
///
// unfolder helper for 20 elements
[[nodiscard]] constexpr auto unfolder20(const auto& aggregate) -> auto
{
  auto&& [t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15, t16, t17, t18, t19, t20] = aggregate;
  return to_tuple(t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15, t16, t17, t18, t19, t20);
}
// unfolder helper for 19 elements
[[nodiscard]] constexpr auto unfolder19(const auto& aggregate) -> auto
{
  auto&& [t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15, t16, t17, t18, t19] = aggregate;
  return to_tuple(t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15, t16, t17, t18, t19);
}
// unfolder helper for 18 elements
[[nodiscard]] constexpr auto unfolder18(const auto& aggregate) -> auto
{
  auto&& [t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15, t16, t17, t18] = aggregate;
  return to_tuple(t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15, t16, t17, t18);
}
// unfolder helper for 17 elements
[[nodiscard]] constexpr auto unfolder17(const auto& aggregate) -> auto
{
  auto&& [t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15, t16, t17] = aggregate;
  return to_tuple(t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15, t16, t17);
}
// unfolder helper for 16 elements
[[nodiscard]] constexpr auto unfolder16(const auto& aggregate) -> auto
{
  auto&& [t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15, t16] = aggregate;
  return to_tuple(t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15, t16);
}
// unfolder helper for 15 elements
[[nodiscard]] constexpr auto unfolder15(const auto& aggregate) -> auto
{
  auto&& [t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15] = aggregate;
  return to_tuple(t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14, t15);
}
// unfolder helper for 14 elements
[[nodiscard]] constexpr auto unfolder14(const auto& aggregate) -> auto
{
  auto&& [t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14] = aggregate;
  return to_tuple(t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13, t14);
}
// unfolder helper for 13 elements
[[nodiscard]] constexpr auto unfolder13(const auto& aggregate) -> auto
{
  auto&& [t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13] = aggregate;
  return to_tuple(t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12, t13);
}
// unfolder helper for 12 elements
[[nodiscard]] constexpr auto unfolder12(const auto& aggregate) -> auto
{
  auto&& [t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12] = aggregate;
  return to_tuple(t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11, t12);
}
// unfolder helper for 11 elements
[[nodiscard]] constexpr auto unfolder11(const auto& aggregate) -> auto
{
  auto&& [t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11] = aggregate;
  return to_tuple(t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11);
}
// unfolder helper for 10 elements
[[nodiscard]] constexpr auto unfolder10(const auto& aggregate) -> auto
{
  auto&& [t1, t2, t3, t4, t5, t6, t7, t8, t9, t10] = aggregate;
  return to_tuple(t1, t2, t3, t4, t5, t6, t7, t8, t9, t10);
}
// unfolder helper for 9 elements
[[nodiscard]] constexpr auto unfolder9(const auto& aggregate) -> auto
{
  auto&& [t1, t2, t3, t4, t5, t6, t7, t8, t9] = aggregate;
  return to_tuple(t1, t2, t3, t4, t5, t6, t7, t8, t9);
}
// unfolder helper for 8 elements
[[nodiscard]] constexpr auto unfolder8(const auto& aggregate) -> auto
{
  auto&& [t1, t2, t3, t4, t5, t6, t7, t8] = aggregate;
  return to_tuple(t1, t2, t3, t4, t5, t6, t7, t8);
}
// unfolder helper for 7 elements
[[nodiscard]] constexpr auto unfolder7(const auto& aggregate) -> auto
{
  auto&& [t1, t2, t3, t4, t5, t6, t7] = aggregate;
  return to_tuple(t1, t2, t3, t4, t5, t6, t7);
}
// unfolder helper for 6 elements
[[nodiscard]] constexpr auto unfolder6(const auto& aggregate) -> auto
{
  auto&& [t1, t2, t3, t4, t5, t6] = aggregate;
  return to_tuple(t1, t2, t3, t4, t5, t6);
}
// unfolder helper for 5 elements
[[nodiscard]] constexpr auto unfolder5(const auto& aggregate) -> auto
{
  auto&& [t1, t2, t3, t4, t5] = aggregate;
  return to_tuple(t1, t2, t3, t4, t5);
}
// unfolder helper for 4 elements
[[nodiscard]] constexpr auto unfolder4(const auto& aggregate) -> auto
{
  auto&& [t1, t2, t3, t4] = aggregate;
  return to_tuple(t1, t2, t3, t4);
}
// unfolder helper for 3 elements
[[nodiscard]] constexpr auto unfolder3(const auto& aggregate) -> auto
{
  auto&& [t1, t2, t3] = aggregate;
  return to_tuple(t1, t2, t3);
}
// unfolder helper for 2 elements
[[nodiscard]] constexpr auto unfolder2(const auto& aggregate) -> auto
{
  auto&& [t1, t2] = aggregate;
  return to_tuple(t1, t2);
}
// unfolder helper for 1 element
[[nodiscard]] constexpr auto unfolder1(const auto& aggregate) -> auto
{
  auto&& [t1] = aggregate;
  return to_tuple(t1);
}

} // namespace detail

///
/// Get the number of parameters that can be used to construct type T.
/// \tparam T type to be constructed
/// \tparam Max maximum number of parameters to be considered
/// \return number of parameters that can be used to construct type T
///
template<typename T, std::size_t Max = 128>
  requires(std::is_aggregate_v<T>)
consteval auto data_member_count() -> std::size_t
{
  constexpr auto retval = [&]<std::size_t... I>(std::index_sequence<I...>)
  { return detail::constructing_params_count_helper<T, (Max - I)...>::type::value; }(std::make_index_sequence<Max>{});
  static_assert(retval != 0 || std::is_default_constructible_v<T>, "construction params count larger than specified max");
  return retval;
}

///
/// Converts an aggregate type into a tuple.
/// \tparam T The type of the aggregate to be converted
/// \return a tuple containing the elements of the aggregate
///
template<typename T>
  requires(std::is_aggregate_v<T>)
[[nodiscard]] constexpr auto to_tuple(const T& a) -> auto
{
  constexpr auto count = data_member_count<T, 20>(); // max 20 elements supported
  {
    // clang-format off
  if      constexpr(count == 20) { return detail::unfolder20(a); }
  else if constexpr(count == 19) { return detail::unfolder19(a); }
  else if constexpr(count == 18) { return detail::unfolder18(a); }
  else if constexpr(count == 17) { return detail::unfolder17(a); }
  else if constexpr(count == 16) { return detail::unfolder16(a); }
  else if constexpr(count == 15) { return detail::unfolder15(a); }
  else if constexpr(count == 14) { return detail::unfolder14(a); }
  else if constexpr(count == 13) { return detail::unfolder13(a); }
  else if constexpr(count == 12) { return detail::unfolder12(a); }
  else if constexpr(count == 11) { return detail::unfolder11(a); }
  else if constexpr(count == 10) { return detail::unfolder10(a); }
  else if constexpr(count == 9 ) { return detail::unfolder9(a); }
  else if constexpr(count == 8 ) { return detail::unfolder8(a); }
  else if constexpr(count == 7 ) { return detail::unfolder7(a); }
  else if constexpr(count == 6 ) { return detail::unfolder6(a); }
  else if constexpr(count == 5 ) { return detail::unfolder5(a); }
  else if constexpr(count == 4 ) { return detail::unfolder4(a); }
  else if constexpr(count == 3 ) { return detail::unfolder3(a); }
  else if constexpr(count == 2 ) { return detail::unfolder2(a); }
  else if constexpr(count == 1 ) { return detail::unfolder1(a); }
  else if constexpr(count == 0 ) { return std::tuple<>{}; }
  else { static_assert(false, "aggregate to complex, cannot be unfolded into tuple"); return; }
    // clang-format on
  }
}

///
/// Deduces the type of a tuple created from an aggregate type.
/// \tparam T The type of the aggregate to be deduced
///
template<typename T>
  requires(std::is_aggregate_v<T>)
using to_tuple_t = decltype(to_tuple(std::declval<T>()));

///
/// Converts a tuple into a struct of type T.
/// \tparam T The type of the struct to be created
/// \return a struct of type T containing the elements of the tuple
///
template<typename T>
[[nodiscard]] constexpr auto from_tuple(auto tuple) -> T
{
  return std::apply(
    [](auto&&... args) { return T{std::forward<decltype(args)>(args)...}; }, std::forward<decltype(tuple)>(tuple)
  );
}

} // namespace bibstd::meta
