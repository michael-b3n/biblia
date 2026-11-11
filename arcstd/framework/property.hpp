#pragma once

#include <functional>
#include <mutex>

namespace arcstd::framework
{

///
/// Thread safe property created from property tree. The value will synchronize with the tree on modification.
/// The `property_tree` object can access private members of this class \see `framework::property_tree`.
///
template<typename T>
class property final
{
  friend class property_tree;

  // Variables
  mutable std::mutex mtx_;
  T value_;
  std::function<void(const T&)> property_tree_update_;

public: // Typedefs
  using value_type = T;

private: // Structors
  property(const value_type& value);

public: // Structors
  property(const property<value_type>& other);
  property(property<value_type>&& other) noexcept;

public: // Operators
  auto operator=(const property<value_type>& other) & -> property<value_type>&;
  auto operator=(property<value_type>&& other) & noexcept -> property<value_type>&;
  auto operator==(const property<value_type>& other) const -> bool;

public: // Accessor
  ///
  /// Get property value.
  /// \return value of property
  ///
  [[nodiscard]] auto value() const -> value_type;

public: // Modifiers
  ///
  /// Set property value. Updates property in tree if registered.
  ///
  auto value(const value_type& value) -> void;

  ///
  /// Exchange property value. Updates property in tree if registered.
  /// \return old property value
  ///
  [[nodiscard]] auto exchange(const value_type& new_value) -> value_type;
};

///
///
template<typename T>
property<T>::property(const value_type& value)
  : value_{value}
{
}

///
///
template<typename T>
property<T>::property(const property<value_type>& other)
{
  const auto lock = std::scoped_lock{other.mtx_};
  value_ = other.value_;
  property_tree_update_ = other.property_tree_update_;
}

///
///
template<typename T>
property<T>::property(property<value_type>&& other) noexcept
{
  const auto lock = std::scoped_lock{other.mtx_};
  value_ = std::move(other.value_);
  property_tree_update_ = std::move(other.property_tree_update_);
}

///
///
template<typename T>
auto property<T>::operator=(const property<value_type>& other) & -> property<value_type>&
{
  if(this != &other)
  {
    const auto lock = std::scoped_lock{mtx_, other.mtx_};
    value_ = other.value_;
    property_tree_update_ = other.property_tree_update_;
  }
  return *this;
}

///
///
template<typename T>
auto property<T>::operator=(property<value_type>&& other) & noexcept -> property<value_type>&
{
  if(this != &other)
  {
    const auto lock = std::scoped_lock{mtx_, other.mtx_};
    value_ = std::move(other.value_);
    property_tree_update_ = std::move(other.property_tree_update_);
  }
  return *this;
}

///
///
template<typename T>
auto property<T>::operator==(const property<value_type>& other) const -> bool
{
  const auto lock = std::scoped_lock{mtx_, other.mtx_};
  return value_ == other.value_;
}

///
///
template<typename T>
auto property<T>::value() const -> value_type
{
  const auto lock = std::scoped_lock{mtx_};
  return value_;
}

///
///
template<typename T>
auto property<T>::value(const value_type& value) -> void
{
  const auto lock = std::scoped_lock{mtx_};
  if(value_ != value)
  {
    value_ = value;
    if(property_tree_update_)
    {
      property_tree_update_(value_);
    }
  }
}

///
///
template<typename T>
auto property<T>::exchange(const value_type& new_value) -> value_type
{
  const auto lock = std::scoped_lock{mtx_};
  const auto old_value = value_;
  if(value_ != new_value)
  {
    value_ = new_value;
    if(property_tree_update_)
    {
      property_tree_update_(value_);
    }
  }
  return old_value;
}

} // namespace arcstd::framework
