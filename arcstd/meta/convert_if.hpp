#pragma once

#include <type_traits>

namespace bibstd::meta
{

///
/// Convert type T to R if `B == true`.
///
template<bool B, typename T, typename R>
struct convert_if
{
  using type = std::conditional_t<B, R, T>;
};
template<bool B, typename T, typename R>
using convert_if_t = typename convert_if<B, T, R>::type;

} // namespace bibstd::meta
