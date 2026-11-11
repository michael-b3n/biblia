#pragma once

#include <concepts>
#include <type_traits>

namespace bibstd::meta
{

///
/// Type trait to check if a type is templated.
///
template<typename T>
struct is_templated final : std::false_type
{};
template<template<typename...> typename T, typename... Args>
struct is_templated<T<Args...>> final : std::true_type
{};
template<typename T>
inline constexpr auto is_templated_v = is_templated<std::remove_cvref_t<T>>::value;

///
/// Type trait that removes the wrapper from a type if it is a wrapper such as std::optional
/// or std::vector, otherwise returns the type unchanged. The wrapper detection decays the type,
/// so const, volatile and reference indicators on the wrapper itself are ignored.
///
template<typename T>
struct remove_wrapper final : std::false_type
{
private:
  template<typename Original_, typename Decayed_>
  struct remove_wrapper_helper final : std::false_type
  {
    using type = Original_;
  };
  template<typename Original_, template<typename...> typename W_, typename T_>
  struct remove_wrapper_helper<Original_, W_<T_>> final : std::true_type
  {
    using type = T_;
  };

public:
  using type = typename remove_wrapper_helper<T, std::decay_t<T>>::type;
};
template<typename T>
using remove_wrapper_t = typename remove_wrapper<T>::type;

///
/// Are same type trait. Checks variadic pack on same type.
///
template<typename... T>
struct are_same : std::true_type
{};
template<typename T1, typename T2, typename... T>
struct are_same<T1, T2, T...>
{
  static constexpr bool value = std::is_same_v<T1, T2> && are_same<T2, T...>::value;
};
template<typename... T>
inline constexpr bool are_same_v = are_same<T...>::value;

///
/// Type trait to make an arithmetic value type unsigned, leaves floating point types untouched.
/// Allows compilation with floating point types. Const and volatile indicators are preserved,
/// references are not accepted because they are neither integral nor floating point.
///
template<typename T>
struct conditional_unsigned;
template<std::integral T>
struct conditional_unsigned<T> final
{
  using type = std::make_unsigned_t<T>;
};
template<std::floating_point T>
struct conditional_unsigned<T> final
{
  using type = T;
};
template<typename T>
using conditional_unsigned_t = typename conditional_unsigned<T>::type;

} // namespace bibstd::meta
