#pragma once

#include <boost/numeric/conversion/cast.hpp>

#include <type_traits>

#define numeric_cast boost::numeric_cast

namespace bibstd::util
{

///
/// Performs a static_cast during compile time and a numeric cast on runtime.
/// \tparam T arithmetic type to cast to
/// \return casted value
///
template<typename T>
[[nodiscard]] constexpr auto numeric_cast_rt(auto&& v) -> T
  requires(std::is_arithmetic_v<T> && std::is_arithmetic_v<std::remove_cvref_t<decltype(v)>>)
{
  if consteval
  {
    return static_cast<T>(std::forward<decltype(v)>(v));
  }
  else
  {
    return numeric_cast<T>(std::forward<decltype(v)>(v));
  }
}

} // namespace bibstd::util
