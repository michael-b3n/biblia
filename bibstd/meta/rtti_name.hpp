#pragma once

#include "bibstd/meta/type_traits.hpp"

#include <string_view>

namespace bibstd::meta
{
namespace detail
{

///
/// Helper for `rtti_name` to retrieve type name using RTTI.
///
template<typename T>
  requires(!meta::is_templated_v<T>)
struct rtti_name_impl final
{
  static constexpr std::string_view value = []()
  {
    static constexpr std::string_view end_pattern = ">::";
    static constexpr std::string_view middle_pattern = "::";
#if defined(__clang__) || defined(__GNUC__)
    static constexpr std::string_view begin_pattern = "detail::rtti_name_impl<";
    static constexpr auto name = std::string_view{__PRETTY_FUNCTION__};
#elifdef _MSC_VER
    static constexpr std::string_view begin_pattern = "detail::rtti_name_impl<struct ";
    static constexpr auto name = std::string_view{__FUNCSIG__};
#endif
    static constexpr auto begin_pattern_start = name.find(begin_pattern);
    static_assert(begin_pattern_start != std::string_view::npos);
    static constexpr auto begin_pattern_pos = begin_pattern_start + begin_pattern.size();

    static constexpr auto end_pattern_pos = name.find(end_pattern, begin_pattern_pos);
    static_assert(end_pattern_pos != std::string_view::npos);

    static constexpr auto potential_begin_pos = name.rfind(middle_pattern, end_pattern_pos);
    static constexpr auto start_pos = potential_begin_pos == std::string_view::npos || potential_begin_pos < begin_pattern_pos
                                        ? begin_pattern_pos
                                        : potential_begin_pos + middle_pattern.size();
    static_assert(start_pos < end_pattern_pos);
    return name.substr(start_pos, end_pattern_pos - start_pos);
  }();
};

} // namespace detail

///
/// Compile time retrieval of type name using RTTI.
///
template<typename T>
  requires(!meta::is_templated_v<T>)
struct rtti_name final
{
  static constexpr auto value = detail::rtti_name_impl<T>::value;
};

} // namespace bibstd::meta
