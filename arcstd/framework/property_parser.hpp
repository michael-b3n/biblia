#pragma once

#include "arcstd/meta/chrono.hpp"
#include "arcstd/meta/contains.hpp"
#include "arcstd/util/enum.hpp"
#include "arcstd/util/ranges.hpp"

#include <boost/property_tree/ptree.hpp>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <format>
#include <string>
#include <vector>

namespace arcstd::framework
{

///
/// All valid property_core types in a pack.
///
using basic_property_value_types = std::tuple<
  bool,
  std::int8_t,
  std::uint8_t,
  std::int16_t,
  std::uint16_t,
  std::int32_t,
  std::uint32_t,
  std::int64_t,
  std::uint64_t,
  float,
  double,
  std::string>;

///
/// Property type concept.
///
template<typename T>
concept basic_property_value_type = meta::contains_v<basic_property_value_types, std::remove_cv_t<T>>;

///
/// Basic property tree type.
///
using property_tree_type = boost::property_tree::ptree;
using property_path_type = property_tree_type::path_type;

///
/// Pair type concept.
///
template<typename T>
concept property_parser_pair_type = requires(T) {
  T::first;
  T::second;
};

///
/// Property parser defines read and write operations from user defined type to basic property value type.
/// The reader returns the value stored in tree if successfully read, else it must return the default value.
/// The writer writes the provided value to the tree.
///
struct property_parser final
{
  // Constants
  static constexpr auto path_name_size = "size";
  static constexpr auto path_name_value = "value";
  static constexpr auto path_name_has_value = "has_value";
  static constexpr auto path_name_first = "first";
  static constexpr auto path_name_second = "second";

  // Implementation
  template<basic_property_value_type T>
  [[nodiscard]] static auto read(const property_path_type& path, const property_tree_type& tree) -> std::optional<T>;
  template<basic_property_value_type T>
  static auto write(const property_path_type& path, property_tree_type& tree, const T& value) -> void;

  template<util::enum_type T>
  [[nodiscard]] static auto read(const property_path_type& path, const property_tree_type& tree) -> std::optional<T>;
  template<util::enum_type T>
  static auto write(const property_path_type& path, property_tree_type& tree, const T& value) -> void;

  template<property_parser_pair_type T>
  [[nodiscard]] static auto read(const property_path_type& path, const property_tree_type& tree) -> std::optional<T>;
  template<property_parser_pair_type T>
  static auto write(const property_path_type& path, property_tree_type& tree, const T& value) -> void;

  template<typename T>
    requires(meta::is_duration_v<T>)
  [[nodiscard]] static auto read(const property_path_type& path, const property_tree_type& tree) -> std::optional<T>;
  template<typename T>
    requires(meta::is_duration_v<T>)
  static auto write(const property_path_type& path, property_tree_type& tree, const T& value) -> void;

  template<typename T>
    requires(std::is_same_v<T, std::filesystem::path>)
  [[nodiscard]] static inline auto read(const property_path_type& path, const property_tree_type& tree) -> std::optional<T>;
  template<typename T>
    requires(std::is_same_v<T, std::filesystem::path>)
  static inline auto write(const property_path_type& path, property_tree_type& tree, const T& value) -> void;

  template<typename Optional>
    requires(std::is_same_v<Optional, std::optional<typename Optional::value_type>>)
  [[nodiscard]] static auto read(const property_path_type& path, const property_tree_type& tree) -> std::optional<Optional>;
  template<typename Optional>
    requires(std::is_same_v<Optional, std::optional<typename Optional::value_type>>)
  static auto write(const property_path_type& path, property_tree_type& tree, const Optional& value) -> void;

  template<typename Vector>
    requires(std::is_same_v<Vector, std::vector<typename Vector::value_type>>)
  [[nodiscard]] static auto read(const property_path_type& path, const property_tree_type& tree) -> std::optional<Vector>;
  template<typename Vector>
    requires(std::is_same_v<Vector, std::vector<typename Vector::value_type>>)
  static auto write(const property_path_type& path, property_tree_type& tree, const Vector& value) -> void;
};

///
///
template<basic_property_value_type T>
auto property_parser::read(const property_path_type& path, const property_tree_type& tree) -> std::optional<T>
{
  const auto v = tree.get_optional<T>(path);
  return v.has_value() ? v.value() : std::optional<T>{};
}

///
///
template<basic_property_value_type T>
auto property_parser::write(const property_path_type& path, property_tree_type& tree, const T& value) -> void
{
  tree.put(path, value);
}

///
///
template<util::enum_type T>
auto property_parser::read(const property_path_type& path, const property_tree_type& tree) -> std::optional<T>
{
  const auto optional_value = tree.get_optional<std::string>(path);
  if(optional_value.has_value())
  {
    if(const auto v = util::to_enum<T>(optional_value.value()); v.has_value())
    {
      return v.value();
    }
  }
  return std::nullopt;
}

///
///
template<util::enum_type T>
auto property_parser::write(const property_path_type& path, property_tree_type& tree, const T& value) -> void
{
  tree.put(path, util::enum_name(value));
}

///
///
template<property_parser_pair_type T>
auto property_parser::read(const property_path_type& path, const property_tree_type& tree) -> std::optional<T>
{
  const auto first = read<typename T::first_type>(path / path_name_first, tree);
  const auto second = read<typename T::second_type>(path / path_name_second, tree);
  return first.has_value() && second.has_value() ? T{first.value(), second.value()} : std::optional<T>{};
}

///
///
template<property_parser_pair_type T>
auto property_parser::write(const property_path_type& path, property_tree_type& tree, const T& value) -> void
{
  write(path / path_name_first, tree, value.first);
  write(path / path_name_second, tree, value.second);
}

///
///
template<typename T>
  requires(meta::is_duration_v<T>)
auto property_parser::read(const property_path_type& path, const property_tree_type& tree) -> std::optional<T>
{
  const auto duration = read<std::int64_t>(path, tree);
  return duration.has_value() ? std::chrono::duration_cast<T>(std::chrono::nanoseconds{duration.value()}) : std::optional<T>{};
}

///
///
template<typename T>
  requires(meta::is_duration_v<T>)
auto property_parser::write(const property_path_type& path, property_tree_type& tree, const T& value) -> void
{
  write(path, tree, std::chrono::duration_cast<std::chrono::nanoseconds>(value).count());
}

///
///
template<typename T>
  requires(std::is_same_v<T, std::filesystem::path>)
auto property_parser::read(const property_path_type& path, const property_tree_type& tree) -> std::optional<T>
{
  const auto path_string = read<std::string>(path, tree);
  return path_string ? std::optional<T>{std::filesystem::path{*path_string}} : std::optional<T>{};
}

///
///
template<typename T>
  requires(std::is_same_v<T, std::filesystem::path>)
auto property_parser::write(const property_path_type& path, property_tree_type& tree, const T& value) -> void
{
  write(path, tree, value.generic_string());
}

///
///
template<typename Optional>
  requires(std::is_same_v<Optional, std::optional<typename Optional::value_type>>)
auto property_parser::read(const property_path_type& path, const property_tree_type& tree) -> std::optional<Optional>
{
  const auto has_value = read<bool>(path / path_name_has_value, tree);
  if(!has_value.has_value())
  {
    return std::nullopt;
  }
  if(*has_value)
  {
    const auto value = read<typename Optional::value_type>(path / path_name_value, tree);
    return value.has_value() ? Optional{value.value()} : std::nullopt;
  }
  else
  {
    return Optional{};
  }
}

///
///
template<typename Optional>
  requires(std::is_same_v<Optional, std::optional<typename Optional::value_type>>)
auto property_parser::write(const property_path_type& path, property_tree_type& tree, const Optional& value) -> void
{
  write(path / path_name_has_value, tree, static_cast<bool>(value.has_value()));
  if(value)
  {
    write(path / path_name_value, tree, value.value());
  }
}

///
///
template<typename Vector>
  requires(std::is_same_v<Vector, std::vector<typename Vector::value_type>>)
auto property_parser::read(const property_path_type& path, const property_tree_type& tree) -> std::optional<Vector>
{
  const auto size = read<std::uint64_t>(path / path_name_size, tree);
  if(!size.has_value())
  {
    return std::nullopt;
  }
  auto retval = Vector(size.value());
  const auto valid = std::ranges::all_of(
    util::ranges::index_view(retval),
    [&](const auto i)
    {
      auto element = read<typename Vector::value_type>(path / std::format("index_{}", i), tree);
      const auto success = element.has_value();
      if(success)
      {
        retval.at(i) = std::move(element.value());
      }
      return success;
    }
  );
  return valid ? retval : std::optional<Vector>{};
}

///
///
template<typename Vector>
  requires(std::is_same_v<Vector, std::vector<typename Vector::value_type>>)
auto property_parser::write(const property_path_type& path, property_tree_type& tree, const Vector& value) -> void
{
  write(path / path_name_size, tree, static_cast<std::uint64_t>(value.size()));
  std::ranges::for_each(
    util::ranges::index_view(value), [&](const auto i) { write(path / std::format("index_{}", i), tree, value.at(i)); }
  );
}

} // namespace arcstd::framework
