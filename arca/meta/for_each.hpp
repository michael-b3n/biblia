#pragma once

#include "arca/meta/pack.hpp"

namespace bibstd::meta
{

///
/// Apply template T on type P or elements of P if P is pack like type.
///
template<typename P, template<typename...> typename T>
struct for_each
{
  using type = T<P>;
};

template<template<typename...> typename P, typename A, template<typename...> typename T>
struct for_each<P<A>, T>
{
  using type = P<T<A>>;
};

template<template<typename...> typename P, typename A, template<typename...> typename T, typename... Args>
struct for_each<P<A, Args...>, T>
{
  using type = add_to_pack_t<T<A>, typename for_each<P<Args...>, T>::type>;
};

///
/// Return type with applied template T on typename P as T<P>.
/// If P is a pack like type T is applied on elements of P as P<T<E_0>, T<E_1>, ... T<E_N>>.
/// \tparam typename P or pack like type P<E_1, ..., E_N>
/// \tparam template T to be applied onto P or elements of P E_i
///
template<typename P, template<typename...> typename T>
using for_each_t = typename for_each<P, T>::type;

} // namespace bibstd::meta
