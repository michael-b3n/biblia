#pragma once

#include "bibstd/framework/property.hpp"
#include "bibstd/framework/property_parser.hpp"
#include "bibstd/util/exception.hpp"

#include <filesystem>
#include <memory>
#include <mutex>

namespace bibstd::framework
{

///
/// Property tree. This class manages saving and loading of properties.
/// \note This class must be created as a shared pointer.
///
class property_tree final : public std::enable_shared_from_this<property_tree>
{
  // Variables
  inline static std::mutex trees_mtx_{};
  inline static std::vector<std::weak_ptr<property_tree>> trees_{};
  mutable std::mutex mtx_;
  std::filesystem::path tree_file_path_;
  property_tree_type tree_;

public: // Typedefs
  using sptr_type = std::shared_ptr<property_tree>;
  using path_type = property_path_type;

public: // Creator
  ///
  /// Property tree creator
  /// \return
  ///
  [[nodiscard]] static auto create(const std::filesystem::path& tree_file_path) -> property_tree::sptr_type;

public: // Structors
  property_tree() = default;
  property_tree(const std::filesystem::path& tree_file_path);
  ~property_tree() noexcept;

public: // Modifiers
  ///
  /// Create a property in a property tree.
  /// If the property tree already has a value, the value of the created property will be the existing value.
  /// If a new value is created, the property and the tree value will be initialized with the `default_value`.
  /// \return the newly created property
  ///
  template<typename T>
  [[nodiscard]] auto create_property(const property_path_type& path, T&& default_value) -> property<T>;
};

///
///
template<typename T>
auto property_tree::create_property(const property_path_type& path, T&& default_value) -> property<T>
{
  const auto lock = std::scoped_lock{mtx_};
  if(path.empty())
  {
    throw util::exception("register property failed: empty path");
  }
  auto prop = property<T>(property_parser::read<T>(path, tree_).value_or(std::forward<decltype(default_value)>(default_value)));
  prop.property_tree_update_ = [sptr = shared_from_this(), path](const T& value)
  { property_parser::write(path, sptr->tree_, value); };
  prop.property_tree_update_(prop.value());
  return prop;
}

} // namespace bibstd::framework
