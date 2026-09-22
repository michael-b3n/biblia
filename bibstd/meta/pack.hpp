#pragma once

#include <type_traits>
#include <utility>

namespace bibstd::meta
{
///
/// packaged trait.
///
template<typename T>
struct is_packable : std::false_type
{};
template<template<typename...> typename T, typename... Args>
struct is_packable<T<Args...>> : std::true_type
{};
template<typename T>
constexpr bool is_pack_v = is_packable<T>::value;

template<typename T>
concept packaged = is_pack_v<T>;

///
/// Unpack all types in pack like type P into template T.
///
template<template<typename...> typename T, packaged P>
struct unpack;

template<template<typename...> typename T, template<typename...> typename P, typename... Args>
struct unpack<T, P<Args...>>
{
  using type = T<Args...>;
};

///
/// Unpack method to unpack a pack P into a template type T<...>.
/// \tparam T<...> variadic templated type
/// \tparam P pack of types
/// \result type of T<P<0>, P<1>, ... P<n>>
///
template<template<typename...> typename T, typename P>
using unpack_t = typename unpack<T, P>::type;

///
/// Add type to pack like type from the left side.
///
template<typename T, packaged P>
struct add_to_pack;

template<typename T, template<typename...> typename P, typename... Args>
struct add_to_pack<T, P<Args...>>
{
  using type = P<T, Args...>;
};

///
/// Add to pack method to add T to a pack P from the left side.
/// \tparam T generic type
/// \tparam P pack of types
/// \result type of pack<T, P<0>, P<1>, ... P<n>>
///
template<typename T, typename P>
using add_to_pack_t = typename add_to_pack<T, P>::type;

///
/// Remove first type from pack.
///
template<packaged P>
struct remove_from_pack;

template<template<typename...> typename P, typename T, typename... Args>
struct remove_from_pack<P<T, Args...>>
{
  using type = P<Args...>;
};

///
/// Remove from pack method to remove T from a pack P from the left side.
/// \tparam P pack of types as pack<P<0>, P<1>, ... P<n>>
/// \result type of pack<P<1>, ... P<n>>
///
template<typename P>
using remove_from_pack_t = typename remove_from_pack<P>::type;

///
/// Combine two pack types to one pack type. Pack type type is of first type.
///
template<packaged P, packaged... Ps>
struct combine_pack;

template<template<typename...> typename P, typename... Ts>
struct combine_pack<P<Ts...>>
{
  using type = P<Ts...>;
};

template<template<typename...> typename P1, template<typename...> typename P2, typename... T1s, typename... T2s, typename... Ps>
struct combine_pack<P1<T1s...>, P2<T2s...>, Ps...>
{
  using type = typename combine_pack<P1<T1s..., T2s...>, Ps...>::type;
};

///
/// Combine pack method to combine pack P1 with pack P2, ... and PN. Type is of pack type P1.
/// \tparam T pack of types
/// \tparam P pack of types
/// \result type of T<T<0>, T<1>, ... T<m>, P<0>, P<1>, ... P<n>>
///
template<packaged P, packaged... Ps>
using combine_pack_t = typename combine_pack<P, Ps...>::type;

///
/// Split a pack like type P into two pack like types.
///
template<packaged P, std::size_t N>
struct split_pack;

template<template<typename...> typename P, typename... Args>
struct split_pack<P<Args...>, 0>
{
  using first_type = P<>;
  using second_type = P<Args...>;
};

template<template<typename...> typename P, typename... Args>
struct split_pack<P<Args...>, sizeof...(Args)>
{
  using first_type = P<Args...>;
  using second_type = P<>;
};

template<template<typename...> typename P, typename... Args, std::size_t N>
  requires(N < sizeof...(Args) && N > 0)
struct split_pack<P<Args...>, N>
{
private:
  using tuple_type = std::tuple<Args...>;

  template<template<typename...> typename P_, typename SequenceFirst_, typename SequenceSecond_>
  struct split_pack_helper;

  template<template<typename...> typename P_, std::size_t... FI_, std::size_t... SI_>
  struct split_pack_helper<P_, std::index_sequence<FI_...>, std::index_sequence<SI_...>>
  {
    using first_type = P_<std::tuple_element_t<FI_, tuple_type>...>;
    using second_type = P_<std::tuple_element_t<SI_ + N, tuple_type>...>;
  };
  using helper_type = split_pack_helper<P, std::make_index_sequence<N>, std::make_index_sequence<sizeof...(Args) - N>>;

public:
  using first_type = typename helper_type::first_type;
  using second_type = typename helper_type::second_type;
};

///
/// Pack types into pack like type N times.
///
template<template<typename...> typename P, typename T, std::size_t N>
struct pack_n_types
{
private:
  template<packaged P_, typename T_, std::size_t N_>
  struct pack_n_types_helper
  {
    using type = typename pack_n_types_helper<add_to_pack_t<T_, P_>, T_, N_ - 1>::type;
  };

  template<packaged P_, typename T_>
  struct pack_n_types_helper<P_, T_, 0>
  {
    using type = P_;
  };

public:
  using type = typename pack_n_types_helper<P<>, T, N>::type;
};

///
/// Get pack like type filled with types T as P<T_1, ..., T_N>.
/// \tparam P template pack type object to be filled with N types of type T
/// \tparam T type that shall be filled into P
/// \tparam N amount of types T in pack like pack P
///
template<template<typename...> typename P, typename T, std::size_t N>
using pack_n_types_t = typename pack_n_types<P, T, N>::type;

///
/// Deduce index of specific type in pack like type.
///
template<packaged P, typename T>
struct type_index;
template<packaged P, typename T>
struct type_index;
template<template<typename...> typename P, typename... Args, typename T>
struct type_index<P<Args...>, T> final
{
  static constexpr std::size_t index = 0;
};
template<template<typename...> typename P, typename Arg, typename... Args, typename T>
struct type_index<P<Arg, Args...>, T> final
{
  static constexpr std::size_t index = std::is_same_v<T, Arg> ? 0 : (1 + type_index<P<Args...>, T>::index);
};

///
/// Get first type index of type T in pack P.
/// \tparam P pack type containing generic types
/// \tparam T type to be searched in pack P
/// \return index of T in pack, if P does not contain T, index is equal to the size of pack
///
template<packaged P, typename T>
constexpr std::size_t type_index_v = type_index<P, T>::index;

///
/// Extract information of a packable type
/// \tparam P pack like type
///
template<typename P>
struct pack_info;

template<template<typename...> typename P, typename T>
struct pack_info<P<T>> final
{
  // Constants
  static constexpr std::size_t size = 1;

  // Templates
  template<std::size_t N>
    requires(N == 0)
  using type_at = T;

  // Typedefs
  using first_type = T;
  using last_type = T;
};

template<template<typename...> typename P, typename T, typename... Args>
struct pack_info<P<T, Args...>> final
{
  // Constants
  static constexpr std::size_t size = sizeof...(Args) + 1;

  // Templates
  template<std::size_t N>
  using type_at = std::tuple_element_t<N, std::tuple<T, Args...>>;

  // Typedefs
  using first_type = T;
  using last_type = type_at<size - 1>;
};

} // namespace bibstd::meta
