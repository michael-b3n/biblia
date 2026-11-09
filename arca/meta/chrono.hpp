#pragma once

#include <chrono>
#include <type_traits>

namespace bibstd::meta
{
namespace detail
{

///
/// std::chrono::duration type trait.
///
template<typename T>
struct is_duration : std::false_type
{};
template<typename T1, typename T2>
struct is_duration<std::chrono::duration<T1, T2>> : std::true_type
{};

} // namespace detail

///
/// True if T is a std::chrono::duration, cv qualifiers and references aside.
///
template<typename T>
constexpr bool is_duration_v = detail::is_duration<std::remove_cvref_t<T>>::value;

} // namespace bibstd::meta
