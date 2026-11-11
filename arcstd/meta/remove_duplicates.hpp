#pragma once

#include "arcstd/meta/pack.hpp"

#include <type_traits>

namespace bibstd::meta
{

///
/// Helper struct to remove duplicated types from a pack of types.
///
template<meta::packaged T>
struct remove_duplicates;

///
/// \see remove_duplicates
///
template<template<typename...> typename P, typename A>
struct remove_duplicates<P<A>>
{
  using type = P<A>;
};

///
/// \see remove_duplicates
///
template<template<typename...> typename P, typename A, typename... Args>
struct remove_duplicates<P<A, Args...>>
{
  using type = std::conditional_t<
    meta::type_index_v<P<Args...>, A> == sizeof...(Args),
    meta::add_to_pack_t<A, typename remove_duplicates<P<Args...>>::type>,
    typename remove_duplicates<P<Args...>>::type>;
};

///
/// Typedef of type with removed duplicates from pack like type P. Of every type appearing
/// more than once the last occurrence is kept.
/// \tparam P pack like type
/// \result pack like type P without duplicated types
///
template<meta::packaged P>
using remove_duplicates_t = remove_duplicates<P>::type;

} // namespace bibstd::meta
