#pragma once

#include "bibstd/meta/type_traits.hpp"
#include "bibstd/util/exception.hpp"
#include "bibstd/util/ranges.hpp"

#include <algorithm>
#include <array>
#include <type_traits>
#include <utility>

namespace bibstd::util
{
namespace detail
{

template<typename T>
struct is_pair : std::false_type
{};
template<typename FirstType, typename SecondType>
struct is_pair<std::pair<FirstType, SecondType>> : std::true_type
{};
template<typename T>
constexpr bool is_pair_v = is_pair<T>::value;

template<typename FirstType, typename SecondType, bool BidirectionalFlag>
concept mappable_types = requires {
  requires !BidirectionalFlag || !std::is_same_v<FirstType, SecondType>;
  requires std::equality_comparable<FirstType>;
  requires !BidirectionalFlag || std::equality_comparable<SecondType>;
};

///
/// Concept to check if type that shall be accessed is explicit for bidirectional maps.
/// Explicit types are required if first and second types are equality comparable.
/// The concept is always true for unidirectional maps.
///
template<typename T, typename F, typename S, bool BidirectionalFlag>
concept explicit_if_equality_comparable =
  !BidirectionalFlag || std::is_same_v<T, std::conditional_t<std::equality_comparable_with<F, S>, F, T>>;

template<typename T, typename F, typename S, bool BidirectionalFlag>
concept comparable_with = explicit_if_equality_comparable<T, F, S, BidirectionalFlag> && std::equality_comparable_with<T, F>;

} // namespace detail

///
/// Const bimap with compile time access to values.
///
template<typename FirstType, typename SecondType, std::size_t N, bool BidirectionalFlag = true>
  requires(detail::mappable_types<FirstType, SecondType, BidirectionalFlag>)
class const_map final
{
  ///
  /// Compile time pointer comparisons are not defined, even though with GCC a constexpr const_map with pairs
  /// of type pair<C, const char*> will compile. Other compilers like clang will not.
  /// The comparable_type templated alias should be used for comparing elements.
  ///
  template<typename C>
  using comparable_type = std::conditional_t<std::is_convertible_v<C, std::string_view>, std::string_view, C>;

  // Variables
  const std::array<std::pair<FirstType, SecondType>, N> map_;

public: // Typedefs
  using value_type = std::remove_const_t<decltype(map_)>::value_type;
  using size_type = std::size_t;
  using first_type = value_type::first_type;
  using second_type = value_type::second_type;
  using const_iterator = typename std::array<value_type, N>::const_iterator;
  using const_reverse_iterator = typename std::array<value_type, N>::const_reverse_iterator;

public: // Constructor
  ///
  /// Constructor of const bimap.
  /// \tparam ...P pack of type pair
  ///
  template<typename... P>
  constexpr const_map(P&&... p)
    requires(meta::are_same_v<std::pair<first_type, second_type>, std::remove_cvref_t<P>...>);

public: // Accessor
  ///
  /// Checks if first value is contained in const bimap.
  /// \tparam F type must be equality comparable with first_type or first_type if first and second types are similar.
  /// \return true, if first value was found, false otherwise
  ///
  template<typename F>
  [[nodiscard]] constexpr auto contains(const F& first) const -> bool
    requires(detail::comparable_with<F, first_type, second_type, BidirectionalFlag>);

  ///
  /// Checks if second value is contained in const bimap.
  /// \tparam F type must be equality comparable with second_type or second_type if first and second types are similar.
  /// \return true, if second value was found, false otherwise
  ///
  template<typename S>
  [[nodiscard]] constexpr auto contains(const S& second) const -> bool
    requires(detail::comparable_with<S, second_type, first_type, BidirectionalFlag> && BidirectionalFlag);

  ///
  /// Access second value corresponding to first value.
  /// \tparam F type must be equality comparable with first_type or first_type if first and second types are similar.
  /// \return const reference to second element of pair corresponding to first
  ///
  template<typename F>
  [[nodiscard]] constexpr auto at(const F& first) const -> const second_type&
    requires(detail::comparable_with<F, first_type, second_type, BidirectionalFlag>);

  ///
  /// Access first value corresponding to second value.
  /// \tparam F type must be equality comparable with second_type or second_type if first and second types are similar.
  /// \return const reference to first element of pair corresponding to second
  ///
  template<typename S>
  [[nodiscard]] constexpr auto at(const S& second) const -> const first_type&
    requires(detail::comparable_with<S, second_type, first_type, BidirectionalFlag> && BidirectionalFlag);

  ///
  /// Tells how many entries are in the bimap.
  /// \return size of bimap
  ///
  [[nodiscard]] constexpr auto size() const -> size_type;

public: // Iterator Overloads
  constexpr auto begin() const -> const_iterator { return map_.begin(); }
  constexpr auto cbegin() const -> const_iterator { return map_.cbegin(); }
  constexpr auto end() const -> const_iterator { return map_.end(); }
  constexpr auto cend() const -> const_iterator { return map_.cend(); }
  constexpr auto rbegin() const -> const_reverse_iterator { return map_.rbegin(); }
  constexpr auto crbegin() const -> const_reverse_iterator { return map_.crbegin(); }
  constexpr auto rend() const -> const_reverse_iterator { return map_.rend(); }
  constexpr auto crend() const -> const_reverse_iterator { return map_.crend(); }

private: // Implementation
  ///
  /// Checks if values are equal using `comparable_type` for comparison.
  /// \return true if equal, false otherwise
  ///
  constexpr auto is_equal(const auto& lhs, const auto& rhs) const -> bool;
};

///
/// Const bimap type check.
///
template<typename T>
struct is_const_map : std::false_type
{};
template<typename FirstType, typename SecondType, std::size_t N>
struct is_const_map<const_map<FirstType, SecondType, N>> : std::true_type
{};
template<typename T>
constexpr bool is_const_map_v = is_const_map<T>::value;

///
/// Const bimap type concept.
///
template<typename T>
concept const_map_type = is_const_map_v<T>;

///
///
template<typename FirstType, typename SecondType, std::size_t N, bool BidirectionalFlag>
  requires(detail::mappable_types<FirstType, SecondType, BidirectionalFlag>)
template<typename... P>
constexpr const_map<FirstType, SecondType, N, BidirectionalFlag>::const_map(P&&... p)
  requires(meta::are_same_v<std::pair<first_type, second_type>, std::remove_cvref_t<P>...>)
  : map_{std::array{std::forward<P>(p)...}}
{
  std::ranges::for_each(
    util::ranges::index_view_to(sizeof...(P)),
    [&](const auto i)
    {
      std::ranges::for_each(
        util::ranges::index_view_between(i + 1, sizeof...(P)),
        [&](const auto j)
        {
          if(is_equal(map_.at(i).first, map_.at(j).first))
          {
            throw util::exception("duplicates in first elements");
          }
          if constexpr(BidirectionalFlag)
          {
            if(is_equal(map_.at(i).second, map_.at(j).second))
            {
              throw util::exception("duplicates in second elements");
            }
          }
        }
      );
    }
  );
}

///
///
template<typename FirstType, typename SecondType, std::size_t N, bool BidirectionalFlag>
  requires(detail::mappable_types<FirstType, SecondType, BidirectionalFlag>)
template<typename F>
constexpr auto const_map<FirstType, SecondType, N, BidirectionalFlag>::contains(const F& first) const -> bool
  requires(detail::comparable_with<F, first_type, second_type, BidirectionalFlag>)
{
  return std::ranges::find_if(map_, [&](const auto& e) { return is_equal(e.first, first); }) != std::cend(map_);
}

///
///
template<typename FirstType, typename SecondType, std::size_t N, bool BidirectionalFlag>
  requires(detail::mappable_types<FirstType, SecondType, BidirectionalFlag>)
template<typename S>
constexpr auto const_map<FirstType, SecondType, N, BidirectionalFlag>::contains(const S& second) const -> bool
  requires(detail::comparable_with<S, second_type, first_type, BidirectionalFlag> && BidirectionalFlag)
{
  return std::ranges::find_if(map_, [&](const auto& e) { return is_equal(e.second, second); }) != std::cend(map_);
}

///
///
template<typename FirstType, typename SecondType, std::size_t N, bool BidirectionalFlag>
  requires(detail::mappable_types<FirstType, SecondType, BidirectionalFlag>)
template<typename F>
constexpr auto const_map<FirstType, SecondType, N, BidirectionalFlag>::at(const F& first) const -> const second_type&
  requires(detail::comparable_with<F, first_type, second_type, BidirectionalFlag>)
{
  const auto iter = std::ranges::find_if(map_, [&](const auto& e) { return is_equal(e.first, first); });
  if(iter == std::cend(map_))
  {
    throw util::exception("first out of range");
  }
  return iter->second;
}

///
///
template<typename FirstType, typename SecondType, std::size_t N, bool BidirectionalFlag>
  requires(detail::mappable_types<FirstType, SecondType, BidirectionalFlag>)
template<typename S>
constexpr auto const_map<FirstType, SecondType, N, BidirectionalFlag>::at(const S& second) const -> const first_type&
  requires(detail::comparable_with<S, second_type, first_type, BidirectionalFlag> && BidirectionalFlag)
{
  const auto iter = std::ranges::find_if(map_, [&](const auto& e) { return is_equal(e.second, second); });
  if(iter == std::cend(map_))
  {
    throw util::exception("second out of range");
  }
  return iter->first;
}

///
///
template<typename FirstType, typename SecondType, std::size_t N, bool BidirectionalFlag>
  requires(detail::mappable_types<FirstType, SecondType, BidirectionalFlag>)
constexpr auto const_map<FirstType, SecondType, N, BidirectionalFlag>::size() const -> size_type
{
  return map_.size();
}

///
///
template<typename FirstType, typename SecondType, std::size_t N, bool BidirectionalFlag>
  requires(detail::mappable_types<FirstType, SecondType, BidirectionalFlag>)
constexpr auto const_map<FirstType, SecondType, N, BidirectionalFlag>::is_equal(const auto& lhs, const auto& rhs) const -> bool
{
  return static_cast<comparable_type<decltype(lhs)>>(lhs) == static_cast<comparable_type<decltype(rhs)>>(rhs);
}

namespace detail
{

///
/// \return the const_map of \p p, use make_const_map or make_const_bimap
///
template<typename F, typename S, std::size_t N, bool BidirectionalFlag, std::size_t... Is>
consteval auto make_const_map_impl(std::pair<F, S> (&&p)[N], [[maybe_unused]] std::index_sequence<Is...> /*sequence*/) -> auto
{
  return const_map<F, S, N, BidirectionalFlag>(std::forward<decltype(p[Is])>(p[Is])...);
}

} // namespace detail

///
/// \return the const_map of \p p, e.g. make_const_map<int, std::string_view>({{0, "e_0"}, {1, "e_1"}})
///
template<typename F, typename S, std::size_t N>
consteval auto make_const_map(std::pair<F, S> (&&p)[N]) -> auto
{
  static constexpr auto unidirectional = false;
  return detail::make_const_map_impl<F, S, N, unidirectional>(std::forward<decltype(p)>(p), std::make_index_sequence<N>{});
}

///
/// \return the const_map of \p p, looked up in both directions
///
template<typename F, typename S, std::size_t N>
consteval auto make_const_bimap(std::pair<F, S> (&&p)[N]) -> auto
{
  static constexpr auto bidirectional = true;
  return detail::make_const_map_impl<F, S, N, bidirectional>(std::forward<decltype(p)>(p), std::make_index_sequence<N>{});
}

} // namespace bibstd::util
